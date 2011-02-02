/**
 * @file   rwlock.h
 * @author Rakesh Iyer
 * @brief  Interface for the reader writer lock implmentation.
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

#ifndef __RWLOCK_H__
#define __RWLOCK_H__

#define RWLOCK_SUCCESS		0
#define RWLOCK_ERROR		-1

typedef struct __lpx_rwlock_t {
    int readers;
} lpx_rwlock_t;
    

int lpx_rwlock_init(lpx_rwlock_t *rwlock);
int lpx_rwlock_destroy(lpx_rwlock_t *rwlock);

int lpx_rwlock_acquire_reader_lock(lpx_rwlock_t *rwlock);
int lpx_rwlock_acquire_reader_lock_timed(lpx_rwlock_t *rwlock, long timeoutMillis);
int lpx_rwlock_release_reader_lock(lpx_rwlock_t *rwlock);

int lpx_rwlock_acquire_writer_lock(lpx_rwlock_t *rwlock);
int lpx_rwlock_acquire_writer_lock_timed(lpx_rwlock_t *rwlock, long timeoutMillis);
int lpx_rwlock_release_writer_lock(lpx_rwlock_t *rwlock);

#endif

