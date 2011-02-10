/**
 * @file   sem.c
 * @author Rakesh Iyer
 * @brief  A semaphore implementation that is built around pthread mutexes and
 *         condition variables. Supposedly better than a posix semaphore.
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

#include "sem.h"

/* Forward declarations of utility functions. */
static struct timespec timeoutToTimespec(long);

/**
 * @brief  Initialize a semaphore. Starts off as fully available.
 * @param  sem      The semaphore to initialize.
 * @param  maxValue The maximum value of the semaphore.
 * @return 0 on success, -1 on failure.
 */
int lpx_sem_init(lpx_semaphore_t *sem, int maxValue)
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
int lpx_sem_destroy(lpx_semaphore_t *sem)
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
int lpx_sem_up(lpx_semaphore_t *sem)
{
    return lpx_sem_up_multiple(sem, 1);
}



/**
 * @brief  Decrement the semaphore value. Will block if the decrement
 *         causes the semaphore to fall below 0.
 * @param  sem   The semaphore to decrement.
 * @return 0 on success, -1 on failure.
 */
int lpx_sem_down(lpx_semaphore_t *sem)
{
    return lpx_sem_down_multiple(sem, 1); 
}

/**
 * @brief  Add or subtract a value from the semaphore.
 * @param  sem   The semaphore to operate on.
 * @param  value The value to add to the semaphore. Negative values can be used
 *               for sem decrements.
 * @return 0 on success -1 otherwise.
 */
int lpx_sem_op(lpx_semaphore_t *sem, int value)
{
    if (value == 0) {
        return SEMAPHORE_FAILURE;
    }

    if (value > 0) {
        return lpx_sem_up_multiple(sem, value);
    } else {
        return lpx_sem_down_multiple(sem, -1 * value);
    }
}

/**
 * @brief  Subtract a value from the semaphore.
 * @param  sem The semaphore to operate on.
 * @param  value The value to subtract from the semaphore.
 * @return 0 on success, -1 on failure.
 */
int lpx_sem_down_multiple(lpx_semaphore_t *sem, int value)
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
    while (sem->value < value) {
        retval = pthread_cond_wait(&sem->sem_cvar, &sem->sem_mutex);
        
        if (retval != 0) {
            return SEMAPHORE_FAILURE;
        }
    }

    // Definitely most certainly have the mutex here.
    sem->value -= value;

    // Unlock the mutex and exit.
    if (pthread_mutex_unlock(&sem->sem_mutex) != 0) {
        return SEMAPHORE_FAILURE;
    }
    
    return SEMAPHORE_SUCCESS;
}

/**
 * @brief  Add a value to the semaphore.
 * @param  sem The semaphore to operate on.
 * @param  value The value to add to the semaphore.
 * @return 0 on success, -1 on failure.
 */
int lpx_sem_up_multiple(lpx_semaphore_t *sem, int value)
{
    if (sem == NULL || sem->initialized != SEMAPHORE_INITIALIZED) {
        return SEMAPHORE_FAILURE;
    }

    // Grab the mutex.
    if (pthread_mutex_lock(&sem->sem_mutex) != 0) {
        return SEMAPHORE_FAILURE;
    }
    
    // Increment the semaphore value.
    sem->value += value;

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
 * @brief  The timed version of sem_op.
 * @param  sem The semaphore to operate on.
 * @param  value The value to add.
 * @param  timeoutMillis Timeout in milliseconds.
 * @return 0 on success, -1 on failure, -2 on timeout.
 */
int lpx_sem_timed_op(lpx_semaphore_t *sem, int value, long timeoutMillis)
{
    if (value == 0) {
        return SEMAPHORE_FAILURE;
    }

    if (value > 0) {
        return lpx_sem_timed_up(sem, value, timeoutMillis);
    } else {
        return lpx_sem_timed_down(sem, -1 * value, timeoutMillis);
    }
}

/**
 * @brief  Decrement the semaphore. Block for as long as the specified timeout.
 * @param  sem The semaphore to decrement.
 * @param  value The value to decrement from the semaphore.
 * @param  timeoutMillis Timeout in milliseconds.
 * @return 0 on success, -1 on failure, -2 on failure.
 */
int lpx_sem_timed_down(lpx_semaphore_t *sem, int value, long timeoutMillis)
{
    struct timespec timeout;
    struct timespec before;
    struct timespec after;
    int retval = 0;

    if (sem == NULL || sem->initialized != SEMAPHORE_INITIALIZED) {
        return SEMAPHORE_FAILURE;
    }

    if (timeoutMillis <= 0) {
        return SEMAPHORE_FAILURE;
    } else {
        timeout = timeoutToTimespec(timeoutMillis);
    }

    // Grab the mutex with a maximum of the specified timeout.
    clock_gettime(CLOCK_REALTIME, &before);
    if ((retval = pthread_mutex_timedlock(&sem->sem_mutex, &timeout)) != 0) {
        if (retval == ETIMEDOUT) {
            return SEMAPHORE_TIMEOUT;
        } else {
            return SEMAPHORE_FAILURE;
        }
    }
    clock_gettime(CLOCK_REALTIME, &after);

    // We measured how long that operation took. Now readjust the timeout
    // such that it holds the remaining time.
    timeoutMillis -= timespecDiffMillis(after, before);
    if (timeoutMillis < 0) {
        pthread_mutex_unlock(&sem->sem_mutex);
        return SEMAPHORE_TIMEOUT;
    }

    // Wait for a time when you are legally allowed to decrement the semaphore.
    while (sem->value < value) {
        timeout = timeoutToTimespec(timeoutMillis);
        clock_gettime(CLOCK_REALTIME, &before);
        retval = pthread_cond_timedwait(&sem->sem_cvar, &sem->sem_mutex, &timeout);
        
        clock_gettime(CLOCK_REALTIME, &after);
        timeoutMillis -= timespecDiffMillis(after, before);
        
        // Cvar wait timed out or failed.
        if (retval == ETIMEDOUT) {
           pthread_mutex_unlock(&sem->sem_mutex);
           return SEMAPHORE_TIMEOUT;
        }
        if (retval != 0) {
           return SEMAPHORE_FAILURE;
        }

        // Cvar wait succeeded but we dont have any more time left. Too bad.
        if (timeoutMillis <= 0) {
            pthread_mutex_unlock(&sem->sem_mutex);
            return SEMAPHORE_TIMEOUT;
        }
    }

    // Definitely most certainly have the mutex here.
    sem->value -= value;

    // Unlock the mutex and exit.
    if (pthread_mutex_unlock(&sem->sem_mutex) != 0) {
        return SEMAPHORE_FAILURE;
    }
    
    return SEMAPHORE_SUCCESS;
}

/**
 * @brief  Timed version of sem_up_multiple. The timeout is not guaranteed to be 
 *         exact in any situation. It depends on more factors than can be controlled
 *         from userspace.
 * @param  sem The semaphore to operate on.
 * @param  value The value to add to the semaphore.
 * @param  timeoutMillis Timeout in milliseconds for the operation.
 * @return 0 on success, -1 on failure, -2 on failure.
 */
int lpx_sem_timed_up(lpx_semaphore_t *sem, int value, long timeoutMillis)
{
    struct timespec timeout;
    int retval = 0;

    if (sem == NULL || sem->initialized != SEMAPHORE_INITIALIZED) {
        return SEMAPHORE_FAILURE;
    }

    if (timeoutMillis <= 0) {
        return SEMAPHORE_FAILURE;
    } else {
        timeout = timeoutToTimespec(timeoutMillis);
    }

    // Grab the mutex.
    if ((retval = pthread_mutex_timedlock(&sem->sem_mutex, &timeout)) != 0) {
        if (retval == ETIMEDOUT) {
            return SEMAPHORE_TIMEOUT;
        } else {
            return SEMAPHORE_FAILURE;
        }
    }
    
    // Increment the semaphore value.
    sem->value += value;

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
 * @brief  Helper method to convert a timout value in milliseconds to a 
 *         struct timespec that is in absolute time.
 * @param  timeoutMillis Time in milliseconds.
 * @return The same value represented in a struct timespec as absolute time.
 */
static struct timespec timeoutToTimespec(long timeoutMillis)
{
    struct timespec time;
    long timeoutSec = 0;
    long timeoutNanos = 0;
     
    memset(&time, 0, sizeof(struct timespec));
    timeoutSec = timeoutMillis / 1000;
    timeoutMillis -= (timeoutSec * 1000);
    timeoutNanos = timeoutMillis * 1000 * 1000;

    clock_gettime(CLOCK_REALTIME, &time);
    time.tv_sec += timeoutSec;
    time.tv_nsec += timeoutNanos;
    if (time.tv_nsec > 1024 * 1024 * 1024) {
        time.tv_nsec %= (1024 * 1024 * 1024);
        time.tv_sec++;
    }

    return time;
}

/**
 * @brief  Subtract 2 timespecs and return the difference in milliseconds.
 * @param  greater The timespec that represents the greater value.
 * @param  lesser  The timespec that represents the lesser value.
 * @return The difference in milliseconds.
 */
long timespecDiffMillis(struct timespec greater, struct timespec lesser)
{
    long diff = 0;
    long nsecDiff = greater.tv_nsec - lesser.tv_nsec;
    if (nsecDiff < 0) {
        greater.tv_nsec--;
        nsecDiff += (1024 * 1024 * 1024);
    }
    diff = nsecDiff / (1000 * 1000);

    diff += (greater.tv_sec - lesser.tv_sec) * 1000;
    return diff;
}

/* EOF */

