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

#include "mempool.h"
#include "rwlock.h"

/**
 * @def   TREEMAP_SUCCESS
 * @brief Represents a success.
 */
#define TREEMAP_SUCCESS		0

/**
 * @def   TREEMAP_ERROR
 * @brief Represents a failure.
 */
#define TREEMAP_ERROR		-1

/**
 * @def   TREEMAP_PROTECTED
 * @brief Represents a treemap protected with a reader writer lock.
 */
#define TREEMAP_PROTECTED	1

/**
 * @def   TREEMAP_UNPROTECTED
 * @brief Represents a treemap that does not use any synchronization.
 */
#define TREEMAP_UNPROTECTED	2

/**
 * @def   COLOR_RED
 * @brief Node is red (in red black tree terminology).
 */
#define COLOR_RED		((int)'r')

/**
 * @def   COLOR_BLACK
 * @brief Node is black (in red black tree terminology).
 */
#define COLOR_BLACK		((int)'b')

/**
 * @brief A node in the red black tree.
 */
typedef struct __rbnode {
    char color;               /**< Is this node red or black? */
    struct __rbnode *parent;  /**< Pointer to the parent.*/
    struct __rbnode *left;    /**< Pointer to the left child. */
    struct __rbnode *right;   /**< Pointer to the right child. */
    long key;                 /**< The key that this node represents. */
    long value;               /**< The value correpsonding to the key. */
}rbnode;

/**
 * @brief A type for the treemap.
 */
typedef struct __lpx_treemap_t {
    lpx_rwlock_t *rwlock;		/**< Mutex to protect the data structure. */
    lpx_mempool_variable_t *pool;       /**< The pool to allocate from. */
    rbnode *head;                       /**< Pointer to the root node of the tree. */
    int (*comparator)(unsigned long, unsigned long); /**< An optional comparator. */
} lpx_treemap_t;

int lpx_treemap_init(lpx_treemap_t *treemap, int isProtected);

int lpx_treemap_init_from_pool(lpx_treemap_t *treemap, int isProtected, lpx_mempool_variable_t *pool);

int lpx_treemap_put(lpx_treemap_t *treemap, unsigned long key, unsigned long value);

int lpx_treemap_get(lpx_treemap_t *treemap, unsigned long key, unsigned long *value);

int lpx_treemap_delete(lpx_treemap_t *treemap, unsigned long key);

int lpx_treemap_destroy(lpx_treemap_t *treemap);

int lpx_treemap_check_rb_conflicts(lpx_treemap_t *treemap);

int lpx_treemap_override_comparator(lpx_treemap_t *treemap, \
                                    int (*comparator)(unsigned long, unsigned long));

#endif
