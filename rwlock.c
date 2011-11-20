/**
 * @file   rwlock.c
 * @author Rakesh Iyer
 * @brief  An implementation of a reader writer lock including a timed version.
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

#include "rwlock.h"

static struct timespec timeoutToTimespec(long);
extern long timespecDiffMillis(struct timespec greater, struct timespec lesser);

/**
 * @brief Initialize the reader writer lock.
 * @param rwlock The reader writer lock to operate on.
 * @return 0 on success, -1 on failure.
 */
int lpx_rwlock_init(lpx_rwlock_t *rwlock)
{
    if (UNLIKELY(rwlock == NULL)) {
        return RWLOCK_ERROR;
    }

    if (pthread_mutex_init(&rwlock->rwlock_mutex, NULL) != 0) {
        return RWLOCK_ERROR;
    }

    if (pthread_cond_init(&rwlock->rwlock_cvar, NULL) != 0) {
        return RWLOCK_ERROR;
    }

    rwlock->value = 0;

    return RWLOCK_SUCCESS;
}

/**
 * @brief Destroy the reader writer lock.
 * @param rwlock The reader writer lock to operate on.
 * @return 0 on success, -1 on failure.
 */
int lpx_rwlock_destroy(lpx_rwlock_t *rwlock)
{
    if (UNLIKELY(rwlock == NULL)) {
        return RWLOCK_ERROR;
    }

    pthread_mutex_destroy(&rwlock->rwlock_mutex);
    pthread_cond_destroy(&rwlock->rwlock_cvar);
    return RWLOCK_SUCCESS;
}

/**
 * @brief Increment the reader count. Will block if writers hold the lock
 *        but will succeed even if other readers hold the lock. Writers cannot
 *        acquire the lock if a non zero number of readers hold the lock.
 * @param rwlock The reader writer lock to operate on.
 * @return 0 on success, -1 on failure.
 */
int lpx_rwlock_acquire_reader_lock(lpx_rwlock_t *rwlock)
{
    int retval = 0;

    if (UNLIKELY(rwlock == NULL)) {
        return RWLOCK_ERROR;
    }

    // Grab the mutex.
    if (pthread_mutex_lock(&rwlock->rwlock_mutex) != 0) {
        return RWLOCK_ERROR;
    }

    // Anything lesser than 0 means that there is a writer present. A value of 0
    // means its fair game for both readers and writers.
    while (rwlock->value < 0) {
        retval = pthread_cond_wait(&rwlock->rwlock_cvar, &rwlock->rwlock_mutex);

	if (retval != 0) {
	    return RWLOCK_ERROR;
	}
    }

    // We have the mutex 
    rwlock->value++;

    // Unlock the mutex. We're screwed if this call fails.
    if (pthread_mutex_unlock(&rwlock->rwlock_mutex) != 0) {
        return RWLOCK_ERROR;
    }

    return RWLOCK_SUCCESS;
}

/**
 * @brief Increment the reader count. Will block if writers hold the lock
 *        but will succeed even if other readers hold the lock. Will fail
 *        if the lock could not be acquired before the timeout expires.
 * @param rwlock The reader writer lock to operate on.
 * @param timeoutMillis The timeout in milliseconds.
 * @return 0 on success, -1 on failure.
 */
int lpx_rwlock_acquire_reader_lock_timed(lpx_rwlock_t *rwlock, long timeoutMillis)
{
    struct timespec timeout;
    struct timespec before;
    struct timespec after;
    int retval = 0;

    if (UNLIKELY(rwlock == NULL)) {
        return RWLOCK_ERROR;
    }

    if (timeoutMillis <= 0) {
        return RWLOCK_ERROR;
    } else {
        timeout = timeoutToTimespec(timeoutMillis);
    }

    // Grab the mutex with a maximum of the specified timeout.
    clock_gettime(CLOCK_REALTIME, &before);
    if ((retval = pthread_mutex_timedlock(&rwlock->rwlock_mutex, &timeout)) != 0) {
        if (retval == ETIMEDOUT) {
            return RWLOCK_TIMEOUT;
        } else {
            return RWLOCK_ERROR;
        }
    }
    clock_gettime(CLOCK_REALTIME, &after);

    // We measured how long that operation took. Now readjust the timeout
    // such that it holds the remaining time.
    timeoutMillis -= timespecDiffMillis(after, before);
    if (timeoutMillis < 0) {
        pthread_mutex_unlock(&rwlock->rwlock_mutex);
        return RWLOCK_TIMEOUT;
    }

    while (rwlock->value < 0) {
        timeout = timeoutToTimespec(timeoutMillis);
        clock_gettime(CLOCK_REALTIME, &before);
        retval = pthread_cond_timedwait(&rwlock->rwlock_cvar, \
	                                &rwlock->rwlock_mutex, &timeout);
        
        clock_gettime(CLOCK_REALTIME, &after);
        timeoutMillis -= timespecDiffMillis(after, before);
        
        // Cvar wait timed out or failed.
        if (retval == ETIMEDOUT) {
           pthread_mutex_unlock(&rwlock->rwlock_mutex);
           return RWLOCK_TIMEOUT;
        } else if (retval != 0) {
           return RWLOCK_ERROR;
        }

        // Cvar wait succeeded but we dont have any more time left. Too bad.
        if (timeoutMillis <= 0) {
            pthread_mutex_unlock(&rwlock->rwlock_mutex);
            return RWLOCK_TIMEOUT;
        }
    }

    // We have the mutex 
    rwlock->value++;

    // Unlock the mutex and exit.
    if (pthread_mutex_unlock(&rwlock->rwlock_mutex) != 0) {
        return RWLOCK_ERROR;
    }

    return RWLOCK_SUCCESS;
}

/**
 * @brief Decrement the reader count. Will potentially allow other writers to
 *        acquire the lock if the reader count reaches 0.
 * @param rwlock The reader writer lock to operate on.
 * @return 0 on success, -1 on failure.
 */
int lpx_rwlock_release_reader_lock(lpx_rwlock_t *rwlock)
{
    if (UNLIKELY(rwlock == NULL)) {
        return RWLOCK_ERROR;
    }

    // Grab the mutex.
    if (pthread_mutex_lock(&rwlock->rwlock_mutex) != 0) {
        return RWLOCK_ERROR;
    }

    // We have the mutex, decrement the lock that we just released. 
    rwlock->value--;

    // Signal the cvar so that someone may wake up.
    if (pthread_cond_signal(&rwlock->rwlock_cvar) != 0) {
        return RWLOCK_ERROR;
    }

    // Unlock the mutex. We're screwed if this call fails.
    if (pthread_mutex_unlock(&rwlock->rwlock_mutex) != 0) {
        return RWLOCK_ERROR;
    }

    return RWLOCK_SUCCESS;
}

/**
 * @brief Increment the writer count and stop other readers and writers
 *        from acquiring the lock. Will block if the lock is held by other
 *        readers or writers.
 * @param rwlock The reader writer lock to operate on.
 * @return 0 on success, -1 on failure.
 */
int lpx_rwlock_acquire_writer_lock(lpx_rwlock_t *rwlock)
{
    int retval = 0;

    if (UNLIKELY(rwlock == NULL)) {
        return RWLOCK_ERROR;
    }

    // Grab the mutex.
    if (pthread_mutex_lock(&rwlock->rwlock_mutex) != 0) {
        return RWLOCK_ERROR;
    }

    while (rwlock->value != 0) {
        retval = pthread_cond_wait(&rwlock->rwlock_cvar, &rwlock->rwlock_mutex);

	if (retval != 0) {
	    return RWLOCK_ERROR;
	}
    }

    // We have the mutex, the value 0 is the only value that we can
    // have out here. We make it -1 to assert that we have the lock
    // in writer mode.
    rwlock->value--;

    // Unlock the mutex. We're screwed if this call fails.
    if (pthread_mutex_unlock(&rwlock->rwlock_mutex) != 0) {
        return RWLOCK_ERROR;
    }

    return RWLOCK_SUCCESS;
}

/**
 * @brief Increment the writer count and stop other readers and writers
 *        from acquiring the lock. Fail out if the timeout expires before
 *        the lock could be acquired.
 * @param rwlock The reader writer lock to operate on.
 * @param timeoutMillis The timeout in milliseconds.
 * @return 0 on success, -1 on failure.
 */
int lpx_rwlock_acquire_writer_lock_timed(lpx_rwlock_t *rwlock, long timeoutMillis)
{
    struct timespec timeout;
    struct timespec before;
    struct timespec after;
    int retval = 0;

    if (UNLIKELY(rwlock == NULL)) {
        return RWLOCK_ERROR;
    }

    if (timeoutMillis <= 0) {
        return RWLOCK_ERROR;
    } else {
        timeout = timeoutToTimespec(timeoutMillis);
    }

    // Grab the mutex with a maximum of the specified timeout.
    clock_gettime(CLOCK_REALTIME, &before);
    if ((retval = pthread_mutex_timedlock(&rwlock->rwlock_mutex, &timeout)) != 0) {
        if (retval == ETIMEDOUT) {
            return RWLOCK_TIMEOUT;
        } else {
            return RWLOCK_ERROR;
        }
    }
    clock_gettime(CLOCK_REALTIME, &after);

    // We measured how long that operation took. Now readjust the timeout
    // such that it holds the remaining time.
    timeoutMillis -= timespecDiffMillis(after, before);
    if (timeoutMillis < 0) {
        pthread_mutex_unlock(&rwlock->rwlock_mutex);
        return RWLOCK_TIMEOUT;
    }

    while (rwlock->value != 0) {
        timeout = timeoutToTimespec(timeoutMillis);
        clock_gettime(CLOCK_REALTIME, &before);
        retval = pthread_cond_timedwait(&rwlock->rwlock_cvar, \
	                                &rwlock->rwlock_mutex, &timeout);
        
        clock_gettime(CLOCK_REALTIME, &after);
        timeoutMillis -= timespecDiffMillis(after, before);
        
        // Cvar wait timed out or failed.
        if (retval == ETIMEDOUT) {
           pthread_mutex_unlock(&rwlock->rwlock_mutex);
           return RWLOCK_TIMEOUT;
        } else if (retval != 0) {
           return RWLOCK_ERROR;
        }

        // Cvar wait succeeded but we dont have any more time left. Too bad.
        if (timeoutMillis <= 0) {
            pthread_mutex_unlock(&rwlock->rwlock_mutex);
            return RWLOCK_TIMEOUT;
        }
    }

    // We have the mutex, the value 0 is the only value that we can
    // have out here. We make it -1 to assert that we have the lock
    // in writer mode.
    rwlock->value--;

    // Unlock the mutex and exit.
    if (pthread_mutex_unlock(&rwlock->rwlock_mutex) != 0) {
        return RWLOCK_ERROR;
    }

    return RWLOCK_SUCCESS;
}

/**
 * @brief Decrement the writer count and let other readers or writers in.
 * @param rwlock The reader writer lock to operate on.
 * @return 0 on success, -1 on failure.
 */
int lpx_rwlock_release_writer_lock(lpx_rwlock_t *rwlock)
{
    if (UNLIKELY(rwlock == NULL)) {
        return RWLOCK_ERROR;
    }

    // Grab the mutex.
    if (pthread_mutex_lock(&rwlock->rwlock_mutex) != 0) {
        return RWLOCK_ERROR;
    }

    // We have the mutex, increment the value, returning it back to 0.
    rwlock->value++;

    // Signal the cvar so that someone may wake up.
    if (pthread_cond_signal(&rwlock->rwlock_cvar) != 0) {
        return RWLOCK_ERROR;
    }

    // Unlock the mutex. We're screwed if this call fails.
    if (pthread_mutex_unlock(&rwlock->rwlock_mutex) != 0) {
        return RWLOCK_ERROR;
    }

    return RWLOCK_SUCCESS;
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

