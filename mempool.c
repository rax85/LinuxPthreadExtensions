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
 * @param  protected Whether to protect the pool with a mutex or not. Only allocations
 *                   on protected pools are thread safe. Use an unprotected pool only
 *                   when you know that you will not be sharing a pool between threads.
 * @return 0 on success, -1 on failure.
 */
int mempool_create_fixed_pool(MempoolFixed *pool, 
                              int objectSize, 
                              int numObjects, 
                              int protected)
{
    long *currentBlockHeader = NULL;
    long storedObjectSize = 0;
    int i = 0;

    if (pool == NULL || objectSize <= 0 || numObjects <= 0) {
        return MEMPOOL_FAILURE;
    }

    // A null in place of the mutex means that the caller didn't need thread safety.
    if (protected == MEMPOOL_PROTECTED) {
        pool->poolMutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
        if (pool->poolMutex == NULL) {
            return MEMPOOL_FAILURE;
        }

        if (0 != pthread_mutex_init(pool->poolMutex, NULL)) {
	    goto cfpool_destroy1;
	}
    } else {
        pool->poolMutex = NULL;
    }

    // Get all the memory needed up front.
    pool->storedObjectSize = objectSize + MEMPOOL_PER_OBJECT_OVERHEAD;
    pool->poolSize = pool->storedObjectSize * numObjects;
    pool->pool = malloc(pool->poolSize);
    if (pool->pool == NULL) {
        goto cfpool_destroy2;
    }

    // Initialize the memory pool. Pay the cost of initializing the data structure
    // upfront so that allocations become deterministic time.
    storedObjectSize = pool->storedObjectSize;
    currentBlockHeader = (long *)pool->pool;
    for (i = 0; i < numObjects - 1; i++) {
	// The header points to the next free block.
        *currentBlockHeader = (long)currentBlockHeader + storedObjectSize;
	currentBlockHeader = (long *)*currentBlockHeader;
    }
    
    // Now null terminate the list and plug it into the free list.
    *currentBlockHeader = 0;
    pool->freeList = pool->pool;

    // Finally, endorse this struct as a valid memory pool.
    pool->magic = MEMPOOL_FIXED_MAGIC;

    return MEMPOOL_SUCCESS;

cfpool_destroy2: if (pool->poolMutex != NULL) { pthread_mutex_destroy(pool->poolMutex); }
cfpool_destroy1: free(pool->poolMutex);
    return MEMPOOL_FAILURE;
}

/**
 * @brief  Allocate an object from a fixed sized pool.
 * @param  pool The pool to allocate from.
 * @return A valid address on success, NULL on failure.
 */
void *mempool_fixed_alloc(MempoolFixed *pool)
{
    void *addr = NULL;
    long *object = NULL;
    
    if (pool == NULL || pool->magic != MEMPOOL_FIXED_MAGIC) {
        return NULL;
    }

    // Lock the mutex if needed.
    if (pool->poolMutex != NULL) {
        if (0 != pthread_mutex_lock(pool->poolMutex)) {
	    return NULL;
	}
    }

    // Grab the first object from the free list.
    object = (long *)pool->freeList;
    if (object == NULL) {
        if (pool->poolMutex != NULL) {
	    pthread_mutex_unlock(pool->poolMutex);
	    return NULL;
	}
    }

    // Ok, got an onject to return, update the free list to point to the next object.
    pool->freeList = (void *)(*object);

    // Prep the object for return to the caller.
    *object = (long)pool;
    addr = (void *)((long)object + MEMPOOL_PER_OBJECT_OVERHEAD);

    // Unlock the mutex if needed.
    if (pool->poolMutex != NULL) {
        if (0 != pthread_mutex_unlock(pool->poolMutex)) {
	    return NULL;
	}
    }

    return addr;
}

/**
 * @brief  Free an object that was allocated on a fixed mem pool.
 * @param  addr The address of the object that needs to be freed.
 * @return 0 on success, -1 on failure.
 */
int mempool_fixed_free(void *addr)
{
    long *object = (long *)((long)addr - MEMPOOL_PER_OBJECT_OVERHEAD);
    MempoolFixed *pool = NULL;

    if (addr == NULL) {
        return MEMPOOL_FAILURE;
    }

    // The first sizeof(long) bytes from the actual start are a back
    // pointer to the pool that this address belongs to.
    pool = (MempoolFixed *)*object;
    if (pool->magic != MEMPOOL_FIXED_MAGIC) {
        return MEMPOOL_FAILURE;
    }

    // Lock the mutex if needed.
    if (pool->poolMutex != NULL) {
        if (0 != pthread_mutex_lock(pool->poolMutex)) {
	    return MEMPOOL_FAILURE;
	}
    }

    // Re-link the object into the free list.
    *object = (long)pool->freeList;
    pool->freeList = object;

    // Unlock the mutex if needed.
    if (pool->poolMutex != NULL) {
        if (0 != pthread_mutex_unlock(pool->poolMutex)) {
	    return MEMPOOL_FAILURE;
	}
    }

    return MEMPOOL_SUCCESS;
}

/**
 * @brief  Destroy all the resources associated with a fixed sized pool.
 * @param  pool The pool to destroy.
 * @return 0 on success, -1 on failure.
 */
int mempool_destroy_fixed_pool(MempoolFixed *pool)
{
    if (pool == NULL) {
        return MEMPOOL_FAILURE;
    }

    pool->magic = 0;

    if (pool->poolMutex != NULL) {
        pthread_mutex_destroy(pool->poolMutex);
    }

    free(pool->pool);

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
    if (pool == NULL) {
        return MEMPOOL_FAILURE;
    }

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
    
    if (pool == NULL) {
        return NULL;
    }

    // Lock the mutex if needed.
    if (pool->poolMutex != NULL) {
        if (0 != pthread_mutex_lock(pool->poolMutex)) {
	    return NULL;
	}
    }

    

    // Unlock the mutex if needed.
    if (pool->poolMutex != NULL) {
        if (0 != pthread_mutex_unlock(pool->poolMutex)) {
	    return NULL;
	}
    }

    return addr;
}

/**
 * @brief  Free an object that was allocated on variable sized memory pool.
 * @param  addr The address of the object to free.
 * @return 0 on success, -1 on failure.
 */
int mempool_variable_free(void *addr)
{
    if (addr == NULL) {
        return MEMPOOL_FAILURE;
    }

#if 0
    // Lock the mutex if needed.
    if (pool->poolMutex != NULL) {
        if (0 != pthread_mutex_lock(pool->poolMutex)) {
	    return MEMPOOL_FAILURE;
	}
    }

    

    // Unlock the mutex if needed.
    if (pool->poolMutex != NULL) {
        if (0 != pthread_mutex_unlock(pool->poolMutex)) {
	    return MEMPOOL_FAILURE;
	}
    }
#endif
 
    return MEMPOOL_SUCCESS;
}

/**
 * @brief  Release all the resources associated with this pool.
 * @param  pool The pool to destroy.
 * @return 0 on success, -1 on failure.
 */
int mempool_destroy_variable_pool(MempoolVariable *pool)
{
    if (pool == NULL) {
        return MEMPOOL_FAILURE;
    }

    return MEMPOOL_SUCCESS;
}


