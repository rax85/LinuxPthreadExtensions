/**
 * @file   threadPool.h
 * @brief  A thread pool wrapper over regular pthreads threads.
 * @author Rakesh Iyer.
 * @bug    Untested.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the Lesser GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "sem.h"

/**
 * @def   THREAD_POOL_SUCCESS
 * @brief Denotes that the threadpool operation succeeded.
 */
#define THREAD_POOL_SUCCESS	0

/**
 * @def   THREAD_POOL_FAILURE
 * @brief Denotes that the threadpool operation failed.
 */
#define THREAD_POOL_FAILURE	-1

/**
 * @def   THREAD_UNAVAILABLE
 * @brief Denotes that the worker is currently doing something.
 */
#define THREAD_UNAVAILABLE	((char)0)
/**
 * @def   THREAD_AVAILABLE
 * @brief Denotes that the worker is currently idle.
 */
#define THREAD_AVAILABLE	((char)1)
/**
 * @def   THREAD_UNINITIALIZED
 * @brief Denotes that the worker hasn't yet been launched.
 */
#define THREAD_UNINITIALIZED	((char)2)

/**
 * @brief A struct to hold everything that the caller needs to wait for the result.
 */
typedef struct __ThreadFuture {
    Semaphore resultAvailable;   /**< Says that the callback result is available. */
    void *result;                /**< Holds the return value of the callback. */
}ThreadFuture;

/**
 * @brief A struct that contains the callback and parameter to a thread.
 */
typedef struct __WorkItem
{
    void *(*callback)(void *);  /**< The callback to run. */
    void *param;                /**< The parameter to the function. */
    ThreadFuture *future;       /**< The future in which the result has to be stored. */
}WorkItem;

/* Forward declaration. */
struct __ThreadPool;

/**
 * @brief A struct to describe a thread in the pool.
 */
typedef struct __Thread {
    pthread_t tid;                  /**< The thread id of the thread. */
    int index;                      /**< The index of this thread in the pool. */
    pthread_cond_t workAvailable;   /**< Signals that work is available. */
    pthread_mutex_t workMutex;      /**< Bogus mutex for the cvar. */
    WorkItem *workItem;             /**< A work item for the worker to process. */
    struct __ThreadPool *parent;    /**< The parent thread pool of this worker. */
}Thread;

/**
 * @brief A struct to describe a pool of threads.
 */
typedef struct __ThreadPool {
    int minThreads;              /**< The minimum number of threads in the pool. */
    int maxThreads;              /**< The maximum number of threads in the pool. */
    int numAlive;                /**< The number of threads alive. */
    Thread **threads;            /**< An array of threads in the pool. */
    char *availability;          /**< Which threads are available. */
    pthread_mutex_t avlblMutex;  /**< Protects the available array. */
    Semaphore threadCounter;     /**< Counts the number of available threads. */
}ThreadPool;

/**
 * @brief An enum to define the types of thread pools.
 */
typedef enum __PoolType 
{
    THREAD_POOL_FIXED,    /**< The number of threads in the pool stays fixed */ 
    THREAD_POOL_VARIABLE  /**< The number of threads in the pool grows & shrinks */
}PoolType;

ThreadPool *threadPoolInit(int minThreads, int maxThreads, PoolType type);
int threadPoolDestroy(ThreadPool *pool);
ThreadFuture *threadPoolExecute(ThreadPool *pool,
                                void *(*callback)(void *),
                                void *param);
int threadPoolJoin(ThreadFuture *future, void **retval);

int getFirstAvailableWorker(ThreadPool *pool);
int signalWorker(Thread *worker);
int addNewWorker(ThreadPool *pool);

void *worker(void *param);

#endif
