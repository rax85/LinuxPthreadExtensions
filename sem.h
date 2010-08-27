/**
 * @file   sem.h
 * @author Rakesh Iyer
 * @brief  Interface for the pthread semaphore implementation.
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

#ifndef __SEM_H__
#define __SEM_H__

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

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
 * @def   SEMAPHORE_TIMEOUT
 * @brief Denotes that an operation timed out.
 */
#define SEMAPHORE_TIMEOUT	-2

/**
 * @def   SEMAPHORE_INITIALIZED
 * @brief Magic value to denote that a semaphore is initialized.
 */
#define SEMAPHORE_INITIALIZED	0xBAC0BAC0

/**
 * @brief A struct that represents a semaphore.
 */
typedef struct __semaphore_t {
    unsigned int initialized;   /**< Stores whether the semaphore is initalized. */
    int value;                  /**< The current value of the semaphore. */
    pthread_mutex_t sem_mutex;  /**< The mutex that protects the value. */
    pthread_cond_t  sem_cvar;   /**< The cvar that threads wait on for the mutex. */
}lpx_semaphore_t;

int lpx_sem_init(lpx_semaphore_t *sem, int maxValue);
int lpx_sem_destroy(lpx_semaphore_t *sem);
int lpx_sem_up(lpx_semaphore_t *sem);
int lpx_sem_down(lpx_semaphore_t *sem);
int lpx_sem_op(lpx_semaphore_t *sem, int value);
int lpx_sem_timed_op(lpx_semaphore_t *sem, int value, long timeoutMillis);
int lpx_sem_up_multiple(lpx_semaphore_t *sem, int);
int lpx_sem_down_multiple(lpx_semaphore_t *sem, int);
int lpx_sem_timed_down(lpx_semaphore_t *sem, int value, long timeoutMillis);
int lpx_sem_timed_up(lpx_semaphore_t *sem, int value, long timeoutMillis);

long timespecDiffMillis(struct timespec greater, struct timespec lesser);
#endif
