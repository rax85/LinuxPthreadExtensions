/**
 * @file   arraylist.h
 * @brief  The interface for the arraylist.
 * @author Rakesh Iyer.
 * @bug    Unimplemented.
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

#ifndef __ARRAYLIST_H__
#define __ARRAYLIST_H__

#include <pthread.h>
#include "rwlock.h"
#include "mempool.h"

#define ARRAYLIST_SUCCESS		0
#define ARRAYLIST_ERROR			-1
#define ARRAYLIST_OOB			-2

#define ARRAYLIST_PROTECTED     	1
#define ARRAYLIST_UNPROTECTED		2


#define ARRAYLIST_DEFAULT_NUMHEADS 	8
#define ARRAYLIST_HEAD_SIZE		128

typedef struct __lpx_arraylist_t {
    lpx_mempool_variable_t *pool;
    lpx_rwlock_t *rwlock;
    long size;
    int numHeads;
    long **heads;
} lpx_arraylist_t;

int lpx_arraylist_init(lpx_arraylist_t *list, int isProtected);
int lpx_arraylist_init_from_pool(lpx_arraylist_t *list, int isProtected,
                                 lpx_mempool_variable_t *pool);
int lpx_arraylist_destroy(lpx_arraylist_t *list);
int lpx_arraylist_get(lpx_arraylist_t *list, long index, long *value);
int lpx_arraylist_set(lpx_arraylist_t *list, long index, long value);
int lpx_arraylist_append(lpx_arraylist_t *list, long value);
int lpx_arraylist_remove(lpx_arraylist_t *list, long index);
int lpx_arraylist_clear(lpx_arraylist_t *list);
long *lpx_arraylist_to_array(lpx_arraylist_t *list);
long lpx_arraylist_size(lpx_arraylist_t *list);
long lpx_arraylist_get_index(lpx_arraylist_t *list, long key);

#endif

