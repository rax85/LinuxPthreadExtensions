/**
 * @file   treemap.h
 * @author Rakesh Iyer
 * @brief  Interface for the treemap implementation.
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

#ifndef __TREEMAP_H__
#define __TREEMAP_H__

#define TREEMAP_SUCCESS		0
#define TREEMAP_ERROR		-1

#define TREEMAP_PROTECTED	1
#define TREEMAP_UNPROTECTED	2

#define COLOR_RED		3
#define COLOR_BLACK		4

#include "mempool.h"

/**
 * @brief A node in the red black tree.
 */
typedef struct __rbnode {
    char color;
    struct __rbnode *parent;
    struct __rbnode *left;
    struct __rbnode *right;
    long key;
    long value;
}rbnode;

/**
 * @brief A type for the treemap.
 */
typedef struct __lpx_treemap_t {
    pthread_mutex_t *mutex;		/**< Mutex to protect the data structure. */
    lpx_mempool_variable_t *pool;       /**< The pool to allocate from. */
    rbnode *head;
} lpx_treemap_t;

int lpx_treemap_init(lpx_treemap_t *treemap, int isProtected);

int lpx_treemap_init_from_pool(lpx_treemap_t *treemap, int isProtected, lpx_mempool_variable_t *pool);

int lpx_treemap_put(lpx_treemap_t *treemap, unsigned long key, unsigned long value);

int lpx_treemap_get(lpx_treemap_t *treemap, unsigned long key, unsigned long *value);

int lpx_treemap_delete(lpx_treemap_t *treemap, unsigned long key);

int lpx_treemap_destroy(lpx_treemap_t *treemap);

#endif
