/**
 * @file   sem.c
 * @author Rakesh Iyer
 * @brief  A semaphore implementation that is built around pthread mutexes and
 *         condition variables. Supposedly better than a posix semaphore.
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

#include "sem.h"

/**
 * @brief  Initialize a semaphore. Starts off as fully available.
 * @param  sem      The semaphore to initialize.
 * @param  maxValue The maximum value of the semaphore.
 * @return 0 on success, -1 on failure.
 */
int sem_init(Semaphore *sem, int maxValue)
{
    /* Validate all the parameters. */
    if (sem == NULL) {
        return SEMAPHORE_FAILURE;
    }

    if (maxValue <= 0) {
        return SEMAPHORE_FAILURE;
    }
   
    /* All parameters valid, initialize the semaphore. */
    sem->value = maxValue;
    
    if (pthread_mutex_init(&sem->sem_mutex, NULL) != 0) {
        return SEMAPHORE_FAILURE;
    }

    if (pthread_cond_init(&sem->sem_cvar, NULL) != 0) {
        return SEMAPHORE_FAILURE;
    }

    /* Mark the semaphore as initalized and return it. */
    sem->initialized = SEMAPHORE_INITIALIZED;
    
    return SEMAPHORE_SUCCESS;
}

/**
 * @brief  Destroy a semaphore.
 * @param  sem The semaphore to destroy.
 * @return 0 on success, -1 on failure.
 */
int sem_destroy(Semaphore *sem)
{
    /* Validate all the parameters. */
    if (sem == NULL) {
        return SEMAPHORE_FAILURE;
    }

    sem->initialized = 0;
    pthread_cond_destroy(&sem->sem_cvar);
    pthread_mutex_destroy(&sem->sem_mutex);
   
    return SEMAPHORE_SUCCESS;
}

/**
 * @brief  Increment the semaphore value. Should never block.
 * @param  sem   The semaphore to increment.
 * @return 0 on success, -1 on failure.
 */
int sem_up(Semaphore *sem)
{
    if (sem == NULL || sem->initialized != SEMAPHORE_INITIALIZED) {
        return SEMAPHORE_FAILURE;
    }

    // Grab the mutex.
    if (pthread_mutex_lock(&sem->sem_mutex) != 0) {
        return SEMAPHORE_FAILURE;
    }
    
    // Increment the semaphore value.
    sem->value += 1;

    // Unlock the mutex.
    if (pthread_mutex_unlock(&sem->sem_mutex) != 0) {
        return SEMAPHORE_FAILURE;
    }

    // Tell someone to wake up.
    if (pthread_cond_signal(&sem->sem_cvar) != 0) {
        pthread_mutex_unlock(&sem->sem_mutex);
        return SEMAPHORE_FAILURE;
    }
    
    return SEMAPHORE_SUCCESS;
}


/**
 * @brief Decrement the semaphore. Block for as long as the specified timeout.
 * @param sem The semaphore to decrement.
 * @param timeoutMillis Timeout in milliseconds.
 * @return 0 on success, -1 on failure.
 */
int sem_timed_down(Semaphore *sem, long timeoutMillis)
{
    if (sem == NULL || sem->initialized != SEMAPHORE_INITIALIZED) {
        return SEMAPHORE_FAILURE;
    }

    if (timeoutMillis <= 0) {
        return SEMAPHORE_FAILURE;
    }
    
    assert(0);
    return SEMAPHORE_SUCCESS;
}

/**
 * @brief  Decrement the semaphore value. Will block if the decrement
 *         causes the semaphore to fall below 0.
 * @param  sem   The semaphore to decrement.
 * @return 0 on success, -1 on failure.
 */
int sem_down(Semaphore *sem)
{
    int retval = 0;

    if (sem == NULL || sem->initialized != SEMAPHORE_INITIALIZED) {
        return SEMAPHORE_FAILURE;
    }
    
    // Grab the mutex.
    if (pthread_mutex_lock(&sem->sem_mutex) != 0) {
        return SEMAPHORE_FAILURE;
    }

    // Wait for a time when you are legally allowed to decrement the semaphore.
    while (sem->value <= 0) {
        retval = pthread_cond_wait(&sem->sem_cvar, &sem->sem_mutex);
        
        if (retval != 0) {
            return SEMAPHORE_FAILURE;
        }
    }

    // Definitely most certainly have the mutex here.
    sem->value -= 1;

    // Unlock the mutex and exit.
    if (pthread_mutex_unlock(&sem->sem_mutex) != 0) {
        return SEMAPHORE_FAILURE;
    }
    
    return SEMAPHORE_SUCCESS;
}

/* EOF */

