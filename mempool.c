/**
 * @file   mempool.c
 * @brief  An implementation of a fixed sized and variable sized pooled memory
 *         allocator. Specifies a mutex protected and unprotected version.
 * @author Rakesh Iyer.
 * @bug    Untested.
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

#include "mempool.h"

/**
 * @brief  Create a memory pool that can allocate fixed sized objects.
 * @param  pool The pool to create.
 * @param  objectSize Size of an individual object in the pool.
 * @param  numObjects The maximum number of objects in the pool.
 * @param  protected Whether to protect the pool with a mutex or not.
 * @return 0 on success, -1 on failure.
 */
int mempool_create_fixed_pool(MempoolFixed *pool, 
                              int objectSize, 
                              int numObjects, 
                              int protected)
{
    return MEMPOOL_SUCCESS;
}

/**
 * @brief  Allocate an object from the pool.
 * @param  pool The pool to allocate from.
 * @return A valid address on success, NULL on failure.
 */
void *mempool_fixed_alloc(MempoolFixed *pool)
{
    void *addr = NULL;
    return addr;
}

/**
 * @brief  Free an object that was allocated on a fixed mem pool.
 * @param  addr The address of the object that needs to be freed.
 * @return 0 on success, -1 on failure.
 */
int mempool_fixed_free(void *addr)
{
    return MEMPOOL_SUCCESS;
}

/**
 * @brief  Destroy all the resources associated with a fixed sized pool.
 * @param  pool The pool to destroy.
 * @return 0 on success, -1 on failure.
 */
int mempool_destroy_fixed_pool(MempoolFixed *pool)
{
    return MEMPOOL_SUCCESS;
}

/**
 * @brief  Create a memory pool that can allocate variable sized objects.
 * @param  pool The pool to allocate from.
 * @param  size The total size of the memory pool.
 * @param  protected Should the pool be protected by a mutex?
 * @return 0 on success, -1 on failure.
 */
int mempool_create_variable_pool(MempoolVariable *pool, int size, int protected)
{
    return MEMPOOL_SUCCESS;
}

/**
 * @brief  Allocate a variable sized object on the memory pool.
 * @param  pool The pool to allocate from.
 * @param  size The size of the object to allocate.
 * @return A valid address on success, NULL on failure.
 */
void *mempool_variable_alloc(MempoolVariable *pool, int size)
{
    void *addr = NULL;
    return addr;
}

/**
 * @brief  Free an object that was allocated on variable sized memory pool.
 * @param  addr The address of the object to free.
 * @return 0 on success, -1 on failure.
 */
int mempool_variable_free(void *addr)
{
    return MEMPOOL_SUCCESS;
}

/**
 * @brief  Release all the resources associated with this pool.
 * @param  pool The pool to destroy.
 * @return 0 on success, -1 on failure.
 */
int mempool_destroy_variable_pool(MempoolVariable *pool)
{
    return MEMPOOL_SUCCESS;
}


