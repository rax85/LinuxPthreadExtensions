/**
 * @file   test.c
 * @author Rakesh Iyer.
 * @brief  Test cases for the thread pool and semaphores.
 */

#include "threadPool.h"
#include "sem.h"
#include "mempool.h"
#include "pcQueue.h"
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
    assert(retval == -2);
    retval = lpx_sem_timed_op(&sem, -2, 5000);
    assert(retval == -2);
    assert(0 == lpx_sem_up(&sem));
    assert(0 == lpx_sem_down(&sem));
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
    lpx_thread_future_t *future = NULL;
    lpx_threadpool_t *pool = lpx_threadpool_init(1, 1, THREAD_POOL_FIXED);
    assert(pool != NULL);
    printf("=======================================\n");

    for (i = 1; i <= 42; i++) {
        arg = (int *)malloc(sizeof(int));
        *arg = i;
        future = lpx_threadpool_execute(pool, threadWorker1, arg);
        assert (future != NULL);
        sleep(1);
        assert(0 == lpx_threadpool_join(future, (void **)&retval));
        assert(*retval == i);
	free(retval);
	retval = NULL;
    }
    assert(sanityCounter == 42);
    assert(0 == lpx_threadpool_destroy(pool));
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
    lpx_thread_future_t *future[42];
    lpx_threadpool_t *pool = lpx_threadpool_init(1, 1, THREAD_POOL_FIXED);
    assert(pool != NULL);
    sanityCounter = 0;
    printf("=======================================\n");

    for (i = 1; i <= 42; i++) {
        arg = (int *)malloc(sizeof(int));
        *arg = i;
        future[i - 1] = lpx_threadpool_execute(pool, threadWorker1, arg);
        assert (future != NULL);
    }

    for (i = 1; i <= 42; i++) {
        assert(0 == lpx_threadpool_join(future[i - 1], (void **)&retval));
        assert(*retval == i);
	free(retval);
	retval = NULL;
    }

    assert(sanityCounter == 42);
    assert(0 == lpx_threadpool_destroy(pool));
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
    lpx_thread_future_t *future[42];
    lpx_threadpool_t *pool = lpx_threadpool_init(42, 42, THREAD_POOL_FIXED);
    assert(pool != NULL);
    sanityCounter = 0;
    printf("=======================================\n");

    for (i = 1; i <= 42; i++) {
        arg = (int *)malloc(sizeof(int));
        *arg = i;
        future[i - 1] = lpx_threadpool_execute(pool, threadWorker1, arg);
        assert (future[i - 1] != NULL);
    }

    for (i = 1; i <= 42; i++) {
        assert(0 == lpx_threadpool_join(future[i - 1], (void **)&retval));
        assert(*retval == i);
	free(retval);
	retval = NULL;
    }

    assert(sanityCounter == 42);
    assert(0 == lpx_threadpool_destroy(pool));
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
    lpx_thread_future_t *future[42];
    lpx_threadpool_t *pool = lpx_threadpool_init(42, 42, THREAD_POOL_FIXED);
    assert(pool != NULL);
    sanityCounter = 0;
    printf("=======================================\n");

    for (i = 1; i <= 42; i++) {
        arg = (int *)malloc(sizeof(int));
        *arg = i;
        future[i - 1] = lpx_threadpool_execute(pool, threadWorker1, arg);
        assert (future[i - 1] != NULL);
    }

    printf("Sleeping before join\n");
    sleep(10);

    for (i = 1; i <= 42; i++) {
        assert(0 == lpx_threadpool_join(future[i - 1], (void **)&retval));
        assert(*retval == i);
	free(retval);
	retval = NULL;
    }

    assert(sanityCounter == 42);
    assert(0 == lpx_threadpool_destroy(pool));
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
    lpx_thread_future_t *future[42];
    lpx_threadpool_t *pool = lpx_threadpool_init(12, 42, THREAD_POOL_VARIABLE);
    assert(pool != NULL);
    sanityCounter = 0;
    printf("=======================================\n");

    for (i = 1; i <= 42; i++) {
        arg = (int *)malloc(sizeof(int));
        *arg = i;
        future[i - 1] = lpx_threadpool_execute(pool, threadWorker1, arg);
        assert (future[i - 1] != NULL);
    }

    for (i = 1; i <= 42; i++) {
        assert(0 == lpx_threadpool_join(future[i - 1], (void **)&retval));
        assert(*retval == i);
	free(retval);
	retval = NULL;
    }

    assert(sanityCounter == 42);
    assert(0 == lpx_threadpool_destroy(pool));
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

char barrierOutArray[BARRIERTEST1_NUM_ITERATIONS * BARRIERTEST1_NUM_THREADS]; /**< Output array for barrierTest1 threads. */
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
    lpx_thread_future_t *future[BARRIERTEST1_NUM_THREADS];
    lpx_barrier_t barrier;

    barrierTest1Index = 0;
    pthread_mutex_init(&barrierTest1Mutex, NULL);

    lpx_threadpool_t *pool = lpx_threadpool_init(BARRIERTEST1_NUM_THREADS, \
                                           BARRIERTEST1_NUM_THREADS, THREAD_POOL_FIXED);
    assert(pool != NULL);

    assert(0 == lpx_create_barrier(&barrier, BARRIERTEST1_NUM_THREADS));
    printf("=======================================\n");

    for (i = 0; i < BARRIERTEST1_NUM_THREADS; i++) {
        future[i] = lpx_threadpool_execute(pool, barrierTest, (void *)&barrier);
        assert (future[i] != NULL);
    }

    for (i = 0; i < BARRIERTEST1_NUM_THREADS; i++) {
        assert(0 == lpx_threadpool_join(future[i], (void **)&retval));
    }

    for (n = 0; n < BARRIERTEST1_NUM_ITERATIONS; n++) {
        for (i = 0; i < BARRIERTEST1_NUM_THREADS; i++) {
            assert(barrierOutArray[index++] == n);
        }
    }

    assert(0 == lpx_threadpool_destroy(pool));
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

#define BASEF_SIZE		4096
#define BASEV_SIZE		4096

/**
 * @brief Run some simple tests on fixed and variable pools.
 * @param baseF The base address for the fixed pool.
 * @param baseV The base address for the variable pool.
 */
void runTests(void *baseF, void *baseV) 
{
    void *fobjects[31];
    void *vobjects[10];
    int i = 0;

    // Create a fixed pool from baseF
    lpx_mempool_fixed_t fpool;
    assert(0 == lpx_mempool_create_fixed_pool_from_block(&fpool, 128, 31, BASEF_SIZE, MEMPOOL_UNPROTECTED, baseF));

    // Allocate a bunch of stuff from the fixed pool.
    for (i = 0; i < 31; i++) {
        fobjects[i] = lpx_mempool_fixed_alloc(&fpool);
	assert(NULL != fobjects[i]);
	memset(fobjects[i], 0, 128);
    }

    // Create a variable pool from baseV
    lpx_mempool_variable_t vpool;
    assert(0 == lpx_mempool_create_variable_pool_from_block(&vpool, BASEV_SIZE, MEMPOOL_UNPROTECTED, baseV));

    // Allocate a bunch of stuff from the variable pool.
    for (i = 0; i < 10; i++) {
        vobjects[i] = lpx_mempool_variable_alloc(&vpool, 128 + i);
	assert(NULL != vobjects[i]);
	memset(vobjects[i], 1, 128 + i);
    }

    // Deallocate fixed.
    for (i = 0; i < 31; i++) {
        lpx_mempool_fixed_free(fobjects[i]);
    }

    // Deallocate variable.
    for (i = 0; i < 10; i++) {
        lpx_mempool_variable_free(vobjects[i]);
    }

    // Don't destroy the fixed pool that we created from the block.
    // Don't destroy the variable pool that we created from the block.
}

/**
 * @brief Test the creation of pools within fixed pools. This is more of a demo
 *        of how to use this functionality really.
 */
void testPoolsFromFixedPool()
{
    lpx_mempool_fixed_t parentPool;
    void *baseF;
    void *baseV;

    printf("=======================================\n");
    assert(0 == lpx_mempool_create_fixed_pool(&parentPool, BASEF_SIZE, 2, MEMPOOL_PROTECTED));
    baseF = lpx_mempool_fixed_alloc(&parentPool);
    assert(baseF != NULL);
    memset(baseF, 0, BASEF_SIZE);
    
    baseV = lpx_mempool_fixed_alloc(&parentPool);
    assert(baseV != NULL);
    memset(baseV, 1, BASEV_SIZE);

    runTests(baseF, baseV);

    assert(0 == lpx_mempool_fixed_free(baseF));
    assert(0 == lpx_mempool_fixed_free(baseV));

    assert(0 == lpx_mempool_destroy_fixed_pool(&parentPool));
    printf("Test testPoolsFromFixedPool passed.\n");
}

/**
 * @brief Test the creation of pools within variable pools. Again, more of a demo.
 */
void testPoolsFromVariablePool()
{
    lpx_mempool_variable_t parentPool;
    void *baseF;
    void *baseV;

    printf("=======================================\n");
    assert(0 == lpx_mempool_create_variable_pool(&parentPool, BASEF_SIZE + BASEV_SIZE + 128, MEMPOOL_PROTECTED));
    baseF = lpx_mempool_variable_alloc(&parentPool, BASEF_SIZE);
    assert(baseF != NULL);
    memset(baseF, 0, BASEF_SIZE);
    
    baseV = lpx_mempool_variable_alloc(&parentPool, BASEV_SIZE);
    assert(baseV != NULL);
    memset(baseV, 1, BASEV_SIZE);

    runTests(baseF, baseV);
    assert(0 == lpx_mempool_variable_free(baseF));
    assert(0 == lpx_mempool_variable_free(baseV));

    assert(0 == lpx_mempool_destroy_variable_pool(&parentPool));
    printf("Test testPoolsFromVariablePool passed.\n");
}


//------------------------ producer consumer queue Tests ------------------------

/**
 * @brief Check the fifo-ness of the producer consumer queue.
 */
void testPcq1()
{
    lpx_pcq_t queue;
    int in1 = 1;
    int in2 = 2;
    int in3 = 3;
    int *out1 = NULL;
    int *out2 = NULL;
    int *out3 = NULL;

    printf("=======================================\n");
    assert(0 == lpx_pcq_init(&queue, 3));

    assert(0 == lpx_pcq_enqueue(&queue, &in1));
    assert(0 == lpx_pcq_enqueue(&queue, &in2));
    assert(0 == lpx_pcq_enqueue(&queue, &in3));

    assert(0 == lpx_pcq_dequeue(&queue, (void **)&out1)); 
    assert(0 == lpx_pcq_dequeue(&queue, (void **)&out2)); 
    assert(0 == lpx_pcq_dequeue(&queue, (void **)&out3));

    assert(in1 == *out1);
    assert(in2 == *out2);
    assert(in3 == *out3);

    assert(0 == lpx_pcq_destroy(&queue));

    printf("Test testPcq1 passed.\n");
}

/**
 * @brief Check that the queue can be utilized completely.
 */
void testPcq2()
{
    lpx_pcq_t queue;
    int in1 = 1;
    int in2 = 2;
    int in3 = 3;
    int in4 = 4;
    int in5 = 5;

    int *out1 = NULL;
    int *out2 = NULL;
    int *out3 = NULL;
    int *out4 = NULL;
    int *out5 = NULL;

    printf("=======================================\n");
    assert(0 == lpx_pcq_init(&queue, 3));

    assert(0 == lpx_pcq_enqueue(&queue, &in1));
    assert(0 == lpx_pcq_enqueue(&queue, &in2));
    assert(0 == lpx_pcq_enqueue(&queue, &in3));

    assert(0 == lpx_pcq_dequeue(&queue, (void **)&out1)); 
    assert(0 == lpx_pcq_dequeue(&queue, (void **)&out2)); 
    
    assert(0 == lpx_pcq_enqueue(&queue, &in4));
    assert(0 == lpx_pcq_enqueue(&queue, &in5));
    
    assert(0 == lpx_pcq_dequeue(&queue, (void **)&out3));
    assert(0 == lpx_pcq_dequeue(&queue, (void **)&out4)); 
    assert(0 == lpx_pcq_dequeue(&queue, (void **)&out5)); 

    assert(in1 == *out1);
    assert(in2 == *out2);
    assert(in3 == *out3);
    assert(in4 == *out4);
    assert(in5 == *out5);

    assert(0 == lpx_pcq_destroy(&queue));

    printf("Test testPcq2 passed.\n");
}

/**
 * @brief Check that enqueues and dequeues time out.
 */
void testTimedPcq1()
{
    lpx_pcq_t queue;
    int in1 = 1;
    int in2 = 2;
    int in3 = 3;
    int in4 = 3;
    int *out1 = NULL;
    int *out2 = NULL;
    int *out3 = NULL;
    int *out4 = NULL;

    printf("=======================================\n");
    assert(0 == lpx_pcq_init(&queue, 3));

    assert(0 == lpx_pcq_timed_enqueue(&queue, &in1, 1000));
    assert(0 == lpx_pcq_timed_enqueue(&queue, &in2, 1000));
    assert(0 == lpx_pcq_timed_enqueue(&queue, &in3, 1000));
    assert(-2 == lpx_pcq_timed_enqueue(&queue, &in4, 1000));

    assert(0 == lpx_pcq_timed_dequeue(&queue, (void **)&out1, 1000)); 
    assert(0 == lpx_pcq_timed_dequeue(&queue, (void **)&out2, 1000)); 
    assert(0 == lpx_pcq_timed_dequeue(&queue, (void **)&out3, 1000));
    assert(-2 == lpx_pcq_timed_dequeue(&queue, (void **)&out4, 1000));

    assert(in1 == *out1);
    assert(in2 == *out2);
    assert(in3 == *out3);
    assert(NULL == out4);

    assert(0 == lpx_pcq_destroy(&queue));

    printf("Test testTimedPcq1 passed.\n");
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
    testPoolsFromFixedPool();
    testPoolsFromVariablePool();
    testPcq1();
    testPcq2();
    testTimedPcq1();
    return 0;
}


