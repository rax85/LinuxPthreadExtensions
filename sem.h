/**
 * @file   sem.h
 * @author Rakesh Iyer
 * @brief  Interface for the pthread semaphore implementation.
 * @bug    Untested.
 */

#ifndef __SEM_H__
#define __SEM_H__

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

/**
 * @def   SEMAPHORE_SUCCESS
 * @brief Denotes that an operation succeeded.
 */
#define SEMAPHORE_SUCCESS	0

/**
 * @def   SEMAPHORE_FAILURE
 * @brief Denotes that an operation failed.
 */
#define SEMAPHORE_FAILURE	-1

/**
 * @def   SEMAPHORE_INITIALIZED
 * @brief Magic value to denote that a semaphore is initialized.
 */
#define SEMAPHORE_INITIALIZED	0xBAC0BAC0

/**
 * @brief A struct that represents a semaphore.
 */
typedef struct __Semaphore {
    unsigned int initialized;   /**< Stores whether the semaphore is initalized. */
    int value;                  /**< The current value of the semaphore. */
    pthread_mutex_t sem_mutex;  /**< The mutex that protects the value. */
    pthread_cond_t  sem_cvar;   /**< The cvar that threads wait on for the mutex. */
}Semaphore;

int sem_init(Semaphore *, int);
int sem_destroy(Semaphore *sem);
int sem_up(Semaphore *);
int sem_down(Semaphore *);
int sem_timed_down(Semaphore *sem, long timeoutMillis);

#endif
