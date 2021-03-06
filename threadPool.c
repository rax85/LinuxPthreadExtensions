/**
 * @file   threadPool.c
 * @brief  A thread pool wrapper over regular pthreads.
 * @author Rakesh Iyer.
 * @bug    Not tested for performance.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "threadPool.h"

/* Forward declarations of internal functions. */
static void *worker(void *param);
static int getFirstAvailableWorker(lpx_threadpool_t *pool);
static int signalWorker(Thread *worker);
static int addNewWorker(lpx_threadpool_t *pool);

/**
 * @brief  Initialize a thread pool.
 * @param  minThreads The minimum number of threads in the pool.
 * @param  maxThreads The maximum number of threads in the pool.
 * @param  type       The type of thread pool.
 * @return A ThreadPool on success, NULL on failure.
 */
lpx_threadpool_t *lpx_threadpool_init(int minThreads, int maxThreads, lpx_pool_type type)
{
    lpx_threadpool_t *pool = NULL;
    int i = 0;

    /* Validate all the input parameters. */

    /* Max threads has to be greater than 0 and minThreads. */
    if (maxThreads <= 0 || maxThreads < minThreads) {
        return NULL;
    }

    /* Min threads must be greater than zero. */
    if (minThreads < 0) {
        return NULL;
    }

    /* Fixed pools must have min and max threads as the same size. */ 
    if ((type == THREAD_POOL_FIXED) && (minThreads != maxThreads)) {
        return NULL;
    }

    /* All parameters are ok, start initializing the thread pool. */
    pool = (lpx_threadpool_t *)malloc(sizeof(lpx_threadpool_t));
    pool->minThreads = minThreads;
    pool->maxThreads = maxThreads;

    if (0 != pthread_mutex_init(&pool->avlblMutex, NULL)) {
        goto pool_destroy1;
    }

    if (SEMAPHORE_SUCCESS != lpx_sem_init(&pool->threadCounter, maxThreads)) {
        goto pool_destroy2; 
    }
    
    pool->threads = (Thread **)malloc(sizeof(Thread *) * maxThreads);
    if (pool->threads == NULL) {
        goto pool_destroy3;
    }

    pool->availability = (char *)malloc(sizeof(char) * maxThreads);
    if (pool->availability == NULL) {
        goto pool_destroy4;
    }

    /* Now create the thread pool. */
    pool->numAlive = 0;
    for (i = 0; i < maxThreads; i++) {
        pool->availability[i] = THREAD_UNINITIALIZED; 
    }

    for (i = 0; i < minThreads; i++) {
        if (THREAD_POOL_SUCCESS != addNewWorker(pool)) {
	    // TODO: Need to do a cleanup here.
	    goto pool_destroy5;
	}
    }

    return pool;

pool_destroy5: free(pool->availability);
pool_destroy4: free(pool->threads);
pool_destroy3: pthread_mutex_destroy(&pool->avlblMutex);
pool_destroy2: lpx_sem_destroy(&pool->threadCounter);
pool_destroy1: free(pool);
   return NULL;
}

/**
 * @brief  Destroy the specified thread pool. Will block for all threads to terminate.
 *         Trying to create threads when this function is executing can result in 
 *         callers possibly segfaulting or hanging forever.
 * @param  pool The thread pool to destroy.
 * @return 0 on success, -1 on failure.
 */
int lpx_threadpool_destroy(lpx_threadpool_t *pool)
{
    int i = 0;
    Thread *runnable = NULL;

    /* Validate parameters. */
    if (pool == NULL) {
        return THREAD_POOL_FAILURE;
    }

    // Wait for all threads to terminate all user actions.
    for (i = 0; i < pool->maxThreads; i++) {
        lpx_sem_down(&pool->threadCounter);
    }

    // Now tell all the threads to die.
    for (i = 0; i < pool->numAlive; i++) {
        runnable = pool->threads[i];
	if (runnable != NULL) {
	    /* Sending a NULL work item to the worker causes it to exit cleanly. */
	    runnable->workItem = NULL;
	    signalWorker(runnable);
	    pthread_join(runnable->tid, NULL);
	}
    }
    
    /* All threads recovered, free up the data structure. */
    pthread_mutex_destroy(&pool->avlblMutex);
    lpx_sem_destroy(&pool->threadCounter);

    for (i = 0; i < pool->numAlive; i++) {
        free(pool->threads[i]);
    }
    free(pool->threads);
    free(pool->availability);
    free(pool);
    return THREAD_POOL_SUCCESS;
}

/**
 * @brief  Grab a thread from the thread pool and execute the callback function. This
 *         call has different behaviour depending on the type of the pool. If the pool
 *         is a fixed size pool and threads are available, then the callback will be
 *         scheduled for execution immediately. If there aren't enough threads then
 *         the call will block. In the variable sized pool case, if there are enough 
 *         live threads to execute, the callback is scheduled immediately. If there
 *         aren't then there are two things that can happen. If the number of live 
 *         threads is lesser than maxThreads, another thread will be spawned and the
 *         callback is scheduled. If the number of threads has maxed out, then the call
 *         will block till a thread does become available.
 * @param  pool     The thread pool from which to grab a thread.
 * @param  callback The callback function to run.
 * @param  param    The param to pass to the callback function.
 * @return A lpx_thread_future_t object that will eventually hold the return value. 
 */
lpx_thread_future_t *lpx_threadpool_execute(lpx_threadpool_t *pool, 
                                            void *(*callback)(void *), void *param)
{
    int runnableIndex = -1;
    Thread *runnable = NULL;
    lpx_thread_future_t *future = NULL;
    WorkItem *workItem = NULL;

    /* Validate all the parameters. */
    if (callback == NULL || pool == NULL) {
        return NULL;
    }

    /* Create a future to store the results in. */
    future = (lpx_thread_future_t *)malloc(sizeof(lpx_thread_future_t));
    if (future == NULL) {
        return NULL;
    }
    
    if (0 != lpx_sem_init(&future->resultAvailable, 1)) {
        free(future);
	return NULL;
    }

    if (0 != lpx_sem_down(&future->resultAvailable)) {
        lpx_sem_destroy(&future->resultAvailable);
        free(future);
        return NULL;
    }

    /* Create a work item for the pool worker to process. */
    workItem = (WorkItem *)malloc(sizeof(WorkItem));
    if (workItem == NULL) {
        free(future);
        return NULL;
    }
    workItem->callback = callback;
    workItem->param = param;
    workItem->future = future;

    /* Wait for a thread to be available to dispatch the work item. */
    if (SEMAPHORE_SUCCESS != lpx_sem_down(&pool->threadCounter)) {
        free(workItem);
	free(future);
        return NULL;
    }

    /* Find the first available thread, tell it what to do and signal it to run. */
    runnableIndex = getFirstAvailableWorker(pool);

    // The semaphore really controls how many threads are there so this loop will
    // eventually succeed. If it doesn't, there is something seriously wrong with
    // the code. In any case, we do a check.
    while (runnableIndex == -1 && pool->numAlive < pool->maxThreads) {
        // Grow if there is room to grow.
        if (0 != addNewWorker(pool)) {
	    // This is a bad situation.
            free(workItem);
	    free(future);
	    return NULL;
	}

        // Find the thread that we just created. 
        runnableIndex = getFirstAvailableWorker(pool);
    }

    if (runnableIndex == -1) {
        free(workItem);
	free(future);
        return NULL;
    }

    runnable = pool->threads[runnableIndex];
    runnable->workItem = workItem;
    
    if (THREAD_POOL_SUCCESS != signalWorker(runnable)) {
        return NULL;
    }

    return future;
}

/**
 * @brief  Wait for the specified thread to complete execution.
 * @param  future The future to join on.
 * @param  retval The value that the thread returned.
 * @return 0 on success, -1 on failure.
 */
int lpx_threadpool_join(lpx_thread_future_t *future, void **retval)
{
    /* Validate all the parameters. */
    if (future == NULL || retval == NULL) {
        return THREAD_POOL_FAILURE;
    }

    /* Wait for the result to be available.*/
    if (0 != lpx_sem_down(&future->resultAvailable)) {
        return THREAD_POOL_FAILURE;
    }

    /* Store the result and destroy the future object. */
    *retval = future->result;
    lpx_sem_destroy(&future->resultAvailable);
    free(future);

    return THREAD_POOL_SUCCESS;
}

/**
 * @brief  Get the first available worker in a thread pool. May grow the current
 *         number of threads if the pool is variable sized.
 * @param  pool The pool from which a worker is desired.
 * @return The index of the first available thread in the pool.
 */
int getFirstAvailableWorker(lpx_threadpool_t *pool)
{
    int index = 0;
    int availableIndex = -1;

    // Grab the mutex that protects the thread availability array.
    if (pthread_mutex_lock(&pool->avlblMutex) != 0) {
        // This is mostly fatal.
	return -1;
    }

    for (index = 0; index < pool->numAlive; index++) {
        // We've got a thread readily available.
	if (pool->availability[index] == THREAD_AVAILABLE) {
	    availableIndex = index;
	    pool->availability[index] = THREAD_UNAVAILABLE;
	    break;
	}
    }

    if (0 != pthread_mutex_unlock(&pool->avlblMutex)) {
        // Damn. This should never have happened.
	return -1;
    }

    return availableIndex;
}

/**
 * @brief  Indicate to the thread worker that there is a workItem to process.
 * @param  worker The thread worker to signal.
 * @return 0 on success, -1 on failure.
 */
int signalWorker(Thread *worker)
{
    if (0 != lpx_sem_up(&worker->workAvailable)) {
        return THREAD_POOL_FAILURE;
    }

    return THREAD_POOL_SUCCESS;
}

/**
 * @brief  Adds a new thread worker to the pool.
 * @param  pool The pool to grow.
 * @return 0 on success -1 on failure.
 */
int addNewWorker(lpx_threadpool_t *pool)
{
    int currentIndex = 0;
    Thread *runnable = NULL;

    /* Dont segfault :) */
    if (pool == NULL) {
        return THREAD_POOL_FAILURE;
    }

    /* Create and initalize a Thread object. */
    runnable = (Thread *)malloc(sizeof(Thread));
    if (runnable == NULL) { return THREAD_POOL_FAILURE; }

    /* Initialize the semaphore and set it to locked so that the thread can wait */
    if (0 != lpx_sem_init(&runnable->workAvailable, 1)) { goto destroy_thread1; }
    if (0 != lpx_sem_down(&runnable->workAvailable)) { goto destroy_thread2; }

    /* Acquire the lock and grow the thread pool. */
    if (0 != pthread_mutex_lock(&pool->avlblMutex)) { goto destroy_thread2; }

    /* Is there room to grow? */
    if (pool->numAlive + 1 > pool->maxThreads) { 
        goto destroy_thread3;
    }
    
    currentIndex = pool->numAlive;
    pool->numAlive++;
    runnable->index = currentIndex;
    runnable->parent = pool;

    /* Finally, fire up a new worker. */
    if (0 != pthread_create(&runnable->tid, NULL, worker, runnable)) {
	goto destroy_thread3;
    }

    pool->threads[currentIndex] = runnable;
    pool->availability[currentIndex] = THREAD_AVAILABLE; 

    pthread_mutex_unlock(&pool->avlblMutex);
    return THREAD_POOL_SUCCESS;

destroy_thread3: pthread_mutex_unlock(&pool->avlblMutex);
destroy_thread2: lpx_sem_destroy(&runnable->workAvailable);
destroy_thread1: free(runnable);
    return THREAD_POOL_FAILURE;
}

/**
 * @brief  The generic skeleton for a thread worker.
 * @param  param A pointer to the Thread object that represents it.
 * @return Ignored.
 */
void *worker(void *param)
{
    Thread *runnable = (Thread *)param;
    lpx_threadpool_t *parent = runnable->parent;
    int index = runnable->index;

    void *retval = NULL;
    WorkItem *workItem = NULL;
    lpx_thread_future_t *future = NULL;

    while (1) {
        // Wait for some work to be available.
        if (0 != lpx_sem_down(&runnable->workAvailable)) {
	    return NULL;
	}

	workItem = runnable->workItem;
	
        // If the work item is NULL, exit cleanly.
	if (workItem == NULL) {
	    break;
	}

	// Ok, there is a work item to process, so run it.
	retval = workItem->callback(workItem->param);

	// Done running the user function, lets set up the returns for the caller.
	future = workItem->future;
        future->result = retval;
	lpx_sem_up(&future->resultAvailable);
	free(workItem);

	// Finally, mark this thread as available.
        if (0 != pthread_mutex_lock(&parent->avlblMutex)) {
	    // This is bad...
	    return NULL;
	}

	parent->availability[index] = THREAD_AVAILABLE;
	if (0 != pthread_mutex_unlock(&parent->avlblMutex)) {
	    // Better die and let things crash out.
	    return NULL;
	}

        if (0 != lpx_sem_up(&parent->threadCounter)) {
            return NULL;
        }
    }

    return NULL;
}

/**
 * @brief  Create a synchronization barrier.
 * @param  barrier The barrier to initialize.
 * @param  numWaiters The number of threads participating in the barrier.
 * @return 0 on success, -1 on failure.
 */
int lpx_create_barrier(lpx_barrier_t *barrier, int numWaiters)
{
    if (barrier == NULL || numWaiters == 0) {
        return THREAD_POOL_FAILURE;
    }

    barrier->numWaiters = numWaiters;
    barrier->barrierFlag = 0;
    barrier->numArrived = 0;

    if (0 != pthread_mutex_init(&barrier->barrierMutex, NULL)) {
        return THREAD_POOL_FAILURE;
    }

    if (0 != pthread_cond_init(&barrier->barrierCvar, NULL)) {
        pthread_mutex_destroy(&barrier->barrierMutex);
        return THREAD_POOL_FAILURE;
    }

    return THREAD_POOL_SUCCESS;
}

/**
 * @brief  Synchronize on a barrier. This is an implementation of a centralized
 *         barrier algorithm _without_ busy waiting. This might be a good match
 *         for cases where the number of threads oversubscribes the number of 
 *         cores. More advanced barrier algorithms will probably be added sometime
 *         in the future.
 * @param  barrier The barrier to synchronize on.
 * @return 0 on success, -1 on failure.
 */
int lpx_barrier_sync(lpx_barrier_t *barrier)
{
    int barrierFlag = 0;

    if (barrier == NULL) {
        return THREAD_POOL_FAILURE;
    }

    // Save the local barrier flag first.
    barrierFlag = barrier->barrierFlag;

    // Now atomically increment the number of waiters.
    if (0 != pthread_mutex_lock(&barrier->barrierMutex)) { return THREAD_POOL_FAILURE; }
    barrier->numArrived++;

    if (barrier->numArrived == barrier->numWaiters) {
        // All of the threads have arrived, reset the number of threads arrived.
	barrier->numArrived = 0;

	// Now flip the barrier flag. See the function documentation for an explanation.
	barrier->barrierFlag = !barrierFlag;

	// Now make everyone runnable.
	if (0 != pthread_cond_broadcast(&barrier->barrierCvar)) {
            return THREAD_POOL_FAILURE;
	}

        // Finally drop the mutex and lets go on with life.
	if (0 != pthread_mutex_unlock(&barrier->barrierMutex)) { 
	    return THREAD_POOL_FAILURE; 
	}
    } else {
        // This condition is true as long as all the threads havent arrived. Till
	// then keep waiting on the cvar.
	while (barrierFlag == barrier->barrierFlag) {
	    pthread_cond_wait(&barrier->barrierCvar, &barrier->barrierMutex);
	}
        
	// The cvar granted you the mutex, promptly drop it.
	if (0 != pthread_mutex_unlock(&barrier->barrierMutex)) {
            return THREAD_POOL_FAILURE;
	}
    }

    return THREAD_POOL_SUCCESS;
}

/**
 * @brief  Destroy a barrier.
 * @param  barrier The barrier to destroy.
 * @return 0 on success, -1 on failure.
 */
int lpx_destroy_barrier(lpx_barrier_t *barrier)
{
    if (barrier == NULL) {
        return THREAD_POOL_FAILURE;
    }
    
    pthread_cond_destroy(&barrier->barrierCvar);
    pthread_mutex_destroy(&barrier->barrierMutex);

    return THREAD_POOL_SUCCESS;
}

/* EOF */
