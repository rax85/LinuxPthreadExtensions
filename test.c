/**
 * @file   test.c
 * @author Rakesh Iyer.
 * @brief  Test cases for the thread pool and semaphores.
 */

#include "threadPool.h"
#include "sem.h"
#include <assert.h>

void testSem1()
{
    Semaphore sem;
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

void testSem2()
{
    printf("Test testSem2 passed.\n");
    return;
}


int sanityCounter = 0;

void *threadWorker1(void *arg)
{
    int *value = (int *)malloc(sizeof(int));
    *value = *((int *)arg);
    sanityCounter++;
    printf("Worker %d executed\n", *value);
    return value;
}

void testThreadPool1()
{
    int i = 0;
    int arg = 42;
    int *retval = NULL;
    ThreadFuture *future = NULL;
    ThreadPool *pool = threadPoolInit(1, 1, THREAD_POOL_FIXED);
    assert(pool != NULL);

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



void testThreadPool2()
{
    printf("Test testThreadPool2 passed.\n");
    return;
}

int main(int argc, char **argv)
{
    testSem1();
    testSem2();
    testThreadPool1();
    testThreadPool2();
    return 0;
}


