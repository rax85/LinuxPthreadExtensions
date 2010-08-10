/**
 * @file   test.c
 * @author Rakesh Iyer.
 * @brief  Test cases for the thread pool and semaphores.
 */

#include "threadPool.h"
#include "sem.h"
#include "mempool.h"
#include <assert.h>


//------------------------------- Semaphore Tests -----------------------------
/**
 * @brief Sanity check for semaphores. Thread pools internally use
 *        semaphores heavily so those there aren't many tests for
 *        semaphores.
 */
void testSem1()
{
    lpx_semaphore_t sem;
    printf("=======================================\n");
    assert(lpx_sem_init(&sem, 1) == 0);
    assert(sem.value == 1);
    lpx_sem_down(&sem);
    assert(sem.value == 0);
    lpx_sem_up(&sem);
    assert(sem.value == 1);
    assert(lpx_sem_destroy(&sem) == 0);
    printf("Test testSem1 passed.\n");
    return;
}

/**
 * @brief Test for timed semaphore lock.
 */
void testSem2()
{
    int retval = 0;
    lpx_semaphore_t sem;
    printf("=======================================\n");
    assert(lpx_sem_init(&sem, 10) == 0);
    // This one must succeed.
    retval = lpx_sem_timed_op(&sem, -10, 1000);
    assert(retval == 0);
    // This one should time out.
    retval = lpx_sem_timed_op(&sem, -2, 5000);
    assert(retval != 0);
    assert(lpx_sem_destroy(&sem) == 0);
    printf("Test testSem2 passed.\n");
    return;
}

/**
 * @brief Test semaphore memory.
 */
void testSem3()
{
    lpx_semaphore_t sem;
    printf("=======================================\n");
    assert(lpx_sem_init(&sem, 1) == 0);
    lpx_sem_down(&sem);
    lpx_sem_up(&sem);
    lpx_sem_up(&sem);
    lpx_sem_op(&sem, -2);
    assert(lpx_sem_destroy(&sem) == 0);
    printf("Test testSem3 passed.\n");
    return;
}

//------------------------------ Thread pool Tests ----------------------------

int sanityCounter = 0; /**< A stupid way to count the number of executions. */

/**
 * @brief  Dummy routine that is used for testing.
 * @param  arg   A pointer to an integer value.
 * @return The same value that was passed in the arg.
 */
void *threadWorker1(void *arg)
{
    int *value = (int *)malloc(sizeof(int));
    *value = *((int *)arg);
    sanityCounter++;
    free(arg);
    printf("Worker %d executed\n", *value);
    return value;
}

/**
 * @brief Sanity check for the thread pool lib.
 */
void testThreadPool1()
{
    int i = 0;
    int *arg = NULL;
    int *retval = NULL;
    ThreadFuture *future = NULL;
    ThreadPool *pool = threadPoolInit(1, 1, THREAD_POOL_FIXED);
    assert(pool != NULL);
    printf("=======================================\n");

    for (i = 1; i <= 42; i++) {
        arg = (int *)malloc(sizeof(int));
        *arg = i;
        future = threadPoolExecute(pool, threadWorker1, arg);
        assert (future != NULL);
        sleep(1);
        assert(0 == threadPoolJoin(future, (void **)&retval));
        assert(*retval == i);
	free(retval);
	retval = NULL;
    }
    assert(sanityCounter == 42);
    assert(0 == threadPoolDestroy(pool));
    printf("Test testThreadPool1 passed.\n");
    return;
}

/**
 * @brief Sanity check for fixed sized pools and multiple thread futures.
 */
void testThreadPool2()
{
    int i = 0;
    int *arg = NULL;
    int *retval = NULL;
    ThreadFuture *future[42];
    ThreadPool *pool = threadPoolInit(1, 1, THREAD_POOL_FIXED);
    assert(pool != NULL);
    sanityCounter = 0;
    printf("=======================================\n");

    for (i = 1; i <= 42; i++) {
        arg = (int *)malloc(sizeof(int));
        *arg = i;
        future[i - 1] = threadPoolExecute(pool, threadWorker1, arg);
        assert (future != NULL);
    }

    for (i = 1; i <= 42; i++) {
        assert(0 == threadPoolJoin(future[i - 1], (void **)&retval));
        assert(*retval == i);
	free(retval);
	retval = NULL;
    }

    assert(sanityCounter == 42);
    assert(0 == threadPoolDestroy(pool));
    printf("Test testThreadPool2 passed.\n");
    return;
}

/**
 * @brief Simple test for fixed sized thread pool.
 */
void testThreadPool3()
{
    int i = 0;
    int *arg = NULL;
    int *retval = NULL;
    ThreadFuture *future[42];
    ThreadPool *pool = threadPoolInit(42, 42, THREAD_POOL_FIXED);
    assert(pool != NULL);
    sanityCounter = 0;
    printf("=======================================\n");

    for (i = 1; i <= 42; i++) {
        arg = (int *)malloc(sizeof(int));
        *arg = i;
        future[i - 1] = threadPoolExecute(pool, threadWorker1, arg);
        assert (future[i - 1] != NULL);
    }

    for (i = 1; i <= 42; i++) {
        assert(0 == threadPoolJoin(future[i - 1], (void **)&retval));
        assert(*retval == i);
	free(retval);
	retval = NULL;
    }

    assert(sanityCounter == 42);
    assert(0 == threadPoolDestroy(pool));
    printf("Test testThreadPool3 passed.\n");
    return;
}

/**
 * @brief Exercise the thread exit before join case in fixed sized pools.
 */
void testThreadPool4()
{
    int i = 0;
    int *arg = NULL;
    int *retval = NULL;
    ThreadFuture *future[42];
    ThreadPool *pool = threadPoolInit(42, 42, THREAD_POOL_FIXED);
    assert(pool != NULL);
    sanityCounter = 0;
    printf("=======================================\n");

    for (i = 1; i <= 42; i++) {
        arg = (int *)malloc(sizeof(int));
        *arg = i;
        future[i - 1] = threadPoolExecute(pool, threadWorker1, arg);
        assert (future[i - 1] != NULL);
    }

    printf("Sleeping before join\n");
    sleep(10);

    for (i = 1; i <= 42; i++) {
        assert(0 == threadPoolJoin(future[i - 1], (void **)&retval));
        assert(*retval == i);
	free(retval);
	retval = NULL;
    }

    assert(sanityCounter == 42);
    assert(0 == threadPoolDestroy(pool));
    printf("Test testThreadPool4 passed.\n");
    return;
}

/**
 * @brief Simple test for variable sized thread pools.
 */
void testThreadPool5()
{
    int i = 0;
    int *arg = NULL;
    int *retval = NULL;
    ThreadFuture *future[42];
    ThreadPool *pool = threadPoolInit(12, 42, THREAD_POOL_VARIABLE);
    assert(pool != NULL);
    sanityCounter = 0;
    printf("=======================================\n");

    for (i = 1; i <= 42; i++) {
        arg = (int *)malloc(sizeof(int));
        *arg = i;
        future[i - 1] = threadPoolExecute(pool, threadWorker1, arg);
        assert (future[i - 1] != NULL);
    }

    for (i = 1; i <= 42; i++) {
        assert(0 == threadPoolJoin(future[i - 1], (void **)&retval));
        assert(*retval == i);
	free(retval);
	retval = NULL;
    }

    assert(sanityCounter == 42);
    assert(0 == threadPoolDestroy(pool));
    printf("Test testThreadPool5 passed.\n");
    return;
}

//--------------------------------- Barrier Tests -----------------------------

/**
 * @def BARRIERTEST1_NUM_ITERATIONS
 * @brief Number of iterations that each thread will make in barrierTest.
 */
#define BARRIERTEST1_NUM_ITERATIONS	128
/**
 * @def BARRIERTEST1_NUM_THREADS
 * @brief Number of threads in barrierTest1
 */
#define BARRIERTEST1_NUM_THREADS	4

char barrierOutArray[BARRIERTEST1_NUM_ITERATIONS]; /**< Output array for barrierTest1 threads. */
pthread_mutex_t barrierTest1Mutex;  /**< Mutex to protect barrierOutArray */
int barrierTest1Index;   /**< Current index to write to in barrierOutArray */

/**
 * @brief Test function for barriers.
 * @param arg Will hold the barrier that we need to synchronize on.
 * @return Ignored.
 */
void *barrierTest(void *arg)
{
    int i = 0;
    lpx_barrier_t *barrier = (lpx_barrier_t *)arg;
    for(i = 0; i < BARRIERTEST1_NUM_ITERATIONS; i++) {
        pthread_mutex_lock(&barrierTest1Mutex);
	barrierOutArray[barrierTest1Index++] = i;
	pthread_mutex_unlock(&barrierTest1Mutex);
	
	lpx_barrier_sync(barrier);
    }
    return NULL;
}

/**
 * @brief Test case for barriers.
 */
void testBarrier1()
{
    int i = 0;
    int n = 0;
    int index = 0;
    int *retval = NULL;
    ThreadFuture *future[BARRIERTEST1_NUM_THREADS];
    lpx_barrier_t barrier;

    barrierTest1Index = 0;
    pthread_mutex_init(&barrierTest1Mutex, NULL);

    ThreadPool *pool = threadPoolInit(BARRIERTEST1_NUM_THREADS, \
                                      BARRIERTEST1_NUM_THREADS, THREAD_POOL_FIXED);
    assert(pool != NULL);

    assert(0 == lpx_create_barrier(&barrier, BARRIERTEST1_NUM_THREADS));
    printf("=======================================\n");

    for (i = 0; i < BARRIERTEST1_NUM_THREADS; i++) {
        future[i] = threadPoolExecute(pool, barrierTest, (void *)&barrier);
        assert (future[i] != NULL);
    }

    for (i = 0; i < BARRIERTEST1_NUM_THREADS; i++) {
        assert(0 == threadPoolJoin(future[i], (void **)&retval));
    }

    for (n = 0; n < BARRIERTEST1_NUM_ITERATIONS; n++) {
        for (i = 0; i < BARRIERTEST1_NUM_THREADS; i++) {
            assert(barrierOutArray[index++] == n);
        }
    }

    assert(0 == threadPoolDestroy(pool));
    assert(0 == lpx_destroy_barrier(&barrier));
    printf("Test testBarrier1 passed.\n");
    return;
}


//---------------------------- fixed mem pool Tests ---------------------------

/**
 * @brief Sanity test for the unprotected fixed sized memory pools.
 */
void testFixedMemPool1()
{
    lpx_mempool_fixed_t pool;
    char *object1;
    char *object2;

    printf("=======================================\n");
    assert(0 == lpx_mempool_create_fixed_pool(&pool, 64, 2, MEMPOOL_UNPROTECTED));
    object1 = lpx_mempool_fixed_alloc(&pool);
    assert(object1 != NULL);
    memset(object1, 0, 64);
    
    object2 = lpx_mempool_fixed_alloc(&pool);
    assert(object2 != NULL);
    memset(object2, 1, 64);

    assert(NULL == lpx_mempool_fixed_alloc(&pool));
    assert(0 == lpx_mempool_fixed_free(object1));
    assert(0 == lpx_mempool_fixed_free(object2));

    assert(NULL != lpx_mempool_fixed_alloc(&pool));
    assert(NULL != lpx_mempool_fixed_alloc(&pool));
    assert(NULL == lpx_mempool_fixed_alloc(&pool));

    assert(0 == lpx_mempool_destroy_fixed_pool(&pool));
    printf("Test testFixedMemPool1 passed.\n");
    return;
}

/**
 * @brief Sanity check the protected fixed sized memory pools.
 */
void testFixedMemPool2()
{
    lpx_mempool_fixed_t pool;
    char *object1;
    char *object2;

    printf("=======================================\n");
    assert(0 == lpx_mempool_create_fixed_pool(&pool, 64, 2, MEMPOOL_PROTECTED));
    object1 = lpx_mempool_fixed_alloc(&pool);
    assert(object1 != NULL);
    memset(object1, 0, 64);
    
    object2 = lpx_mempool_fixed_alloc(&pool);
    assert(object2 != NULL);
    memset(object2, 1, 64);

    assert(NULL == lpx_mempool_fixed_alloc(&pool));
    assert(0 == lpx_mempool_fixed_free(object1));
    assert(0 == lpx_mempool_fixed_free(object2));

    assert(NULL != lpx_mempool_fixed_alloc(&pool));
    assert(NULL != lpx_mempool_fixed_alloc(&pool));
    assert(NULL == lpx_mempool_fixed_alloc(&pool));

    assert(0 == lpx_mempool_destroy_fixed_pool(&pool));
    printf("Test testFixedMemPool2 passed.\n");
}



//---------------------------- fixed mem pool Tests ---------------------------

/**
 * @brief Sanity test for unprotected variable sized memory pools.
 */
void testVariableMemPool1()
{
    lpx_mempool_variable_t pool;
    char *object1 = NULL;
    char *object2 = NULL;
    long sixM = 6L * 1024L * 1024L;

    printf("=======================================\n");
    assert(0 == lpx_mempool_create_variable_pool(&pool, sixM, MEMPOOL_UNPROTECTED));

    object1 = lpx_mempool_variable_alloc(&pool, 64);
    assert(NULL != object1);
    memset(object1, 1, 64);

    object2 = lpx_mempool_variable_alloc(&pool, 128);
    assert(NULL != object2);
    memset(object2, 2, 128);

    assert(0 == lpx_mempool_variable_free(object1));
    assert(0 == lpx_mempool_variable_free(object2));

    object1 = lpx_mempool_variable_alloc(&pool, sixM);
    assert(NULL != object1);
    memset(object1, 3, sixM);
    assert(0 == lpx_mempool_variable_free(object1));

    assert(0 == lpx_mempool_destroy_variable_pool(&pool));
    printf("Test testVariableMemPool1 passed.\n");
}

/**
 * @brief Sanity test for protected variable sized memory pools.
 */
void testVariableMemPool2()
{
    lpx_mempool_variable_t pool;
    char *object1 = NULL;
    char *object2 = NULL;
    long sixM = 6L * 1024L * 1024L;

    printf("=======================================\n");
    assert(0 == lpx_mempool_create_variable_pool(&pool, sixM, MEMPOOL_PROTECTED));

    object1 = lpx_mempool_variable_alloc(&pool, 64);
    assert(NULL != object1);
    memset(object1, 1, 64);

    object2 = lpx_mempool_variable_alloc(&pool, 128);
    assert(NULL != object2);
    memset(object2, 2, 128);

    assert(0 == lpx_mempool_variable_free(object1));
    assert(0 == lpx_mempool_variable_free(object2));

    object1 = lpx_mempool_variable_alloc(&pool, sixM);
    assert(NULL != object1);
    memset(object1, 3, sixM);
    assert(0 == lpx_mempool_variable_free(object1));

    assert(0 == lpx_mempool_destroy_variable_pool(&pool));
    printf("Test testVariableMemPool2 passed.\n");
}


/**
 * @brief Entry point into the test program.
 * @param argc Number of arguments.
 * @param argv Array of pointers to arguments.
 * @return Always 0.
 */
int main(int argc, char **argv)
{
    testSem1();
    testSem2();
    testSem3();
    testThreadPool1();
    testThreadPool2();
    testThreadPool3();
    testThreadPool4();
    testThreadPool5();
    testBarrier1();
    testFixedMemPool1();
    testFixedMemPool2();
    testVariableMemPool1();
    testVariableMemPool2();
    return 0;
}


