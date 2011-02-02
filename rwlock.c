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

/**
 * @brief Initialize the reader writer lock.
 * @param rwlock The reader writer lock to operate on.
 * @return 0 on success, -1 on failure.
 */
int lpx_rwlock_init(lpx_rwlock_t *rwlock)
{
    return RWLOCK_SUCCESS;
}

/**
 * @brief Destroy the reader writer lock.
 * @param rwlock The reader writer lock to operate on.
 * @return 0 on success, -1 on failure.
 */
int lpx_rwlock_destroy(lpx_rwlock_t *rwlock)
{
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
    return RWLOCK_SUCCESS;
}

/**
 * @brief Decrement the writer count and let other readers or writers in.
 * @param rwlock The reader writer lock to operate on.
 * @return 0 on success, -1 on failure.
 */
int lpx_rwlock_release_writer_lock(lpx_rwlock_t *rwlock)
{
    return RWLOCK_SUCCESS;
}

