/**
 * @file   test.c
 * @author Rakesh Iyer.
 * @brief  Test cases for the thread pool and semaphores.
 */

#include "threadPool.h"
#include "sem.h"
#include <assert.h>

/**
 * @brief Sanity check for semaphores. Thread pools internally use
 *        semaphores heavily so those there aren't many tests for
 *        semaphores.
 */
void testSem1()
{
    Semaphore sem;
    printf("=======================================\n");
    assert(sem_init(&sem, 1) == 0);
    assert(sem.value == 1);
    sem_down(&sem);
    assert(sem.value == 0);
    sem_up(&sem);
    assert(sem.value == 1);
    assert(sem_destroy(&sem) == 0);
    printf("Test testSem1 passed.\n");
    return;
}

/**
 * @brief Placeholder for a test for timed semaphore lock.
 */
void testSem2()
{
    printf("=======================================\n");
    printf("Test testSem2 passed.\n");
    return;
}


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
    printf("Worker %d executed\n", *value);
    return value;
}

/**
 * @brief Sanity check for the thread pool lib.
 */
void testThreadPool1()
{
    int i = 0;
    int arg = 42;
    int *retval = NULL;
    ThreadFuture *future = NULL;
    ThreadPool *pool = threadPoolInit(1, 1, THREAD_POOL_FIXED);
    assert(pool != NULL);
    printf("=======================================\n");

    for (i = 1; i <= 42; i++) {
        arg = i;
        future = threadPoolExecute(pool, threadWorker1, &arg);
        assert (future != NULL);
        sleep(1);
        assert(0 == threadPoolJoin(future, (void **)&retval));
        assert(*retval == arg);
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
    testThreadPool1();
    testThreadPool2();
    testThreadPool3();
    testThreadPool4();
    testThreadPool5();
    return 0;
}


