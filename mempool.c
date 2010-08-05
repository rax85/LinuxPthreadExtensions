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
                              long objectSize, 
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
int mempool_create_variable_pool(MempoolVariable *pool, long size, int protected)
{
    long *blockMetadata = NULL;

    if (pool == NULL) {
        return MEMPOOL_FAILURE;
    }

    if (protected == MEMPOOL_PROTECTED) {
        pool->poolMutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	if (pool->poolMutex == NULL) {
	    return MEMPOOL_FAILURE;
	}
     
        if (0 != pthread_mutex_init(pool->poolMutex, NULL)) {
	    free(pool->poolMutex);
	    return MEMPOOL_FAILURE;
	}
    } else {
        pool->poolMutex = NULL;
    }

    pool->pool = malloc(size);
    if (pool->pool == NULL) {
        if (NULL != pool->poolMutex) {
	    pthread_mutex_destroy(pool->poolMutex);
	    return MEMPOOL_FAILURE;
	}
    }
    pool->poolSize = size;
    pool->freeList = pool->pool;

    // Set up the block metadata.
    blockMetadata = (long *)pool->freeList;
    blockMetadata[VPMD_SIZE_OFFSET] = pool->poolSize;
    blockMetadata[VPMD_PREV_OFFSET] = 0; // NULL the prev pointer.
    blockMetadata[VPMD_NEXT_OFFSET] = 0; // NULL the next pointer.
    
    // Finally, set the flag to indicate that this pool is valid.
    pool->magic = MEMPOOL_VARIABLE_MAGIC;

    return MEMPOOL_SUCCESS;
}

/**
 * @brief  Allocate a variable sized object on the memory pool.
 * @param  pool The pool to allocate from.
 * @param  size The size of the object to allocate.
 * @return A valid address on success, NULL on failure.
 */
void *mempool_variable_alloc(MempoolVariable *pool, long size)
{
    void *candidateBlock = NULL;
    int unlockNeeded = 0;
    
    if (pool == NULL) {
        return NULL;
    }

    // Lock the mutex if needed.
    if (pool->poolMutex != NULL) {
        if (0 != pthread_mutex_lock(pool->poolMutex)) {
	    return NULL;
	}
	unlockNeeded = 1;
    }

    // First off, adjust the size to accomodate the metadata.
    size += MEMPOOL_PER_BLOCK_OVERHEAD;
    if (size < 3 * sizeof(long)) {
        // Because this should be linkable back into the list.
	size = 3 * sizeof(long);
    }

    // Find the first fit block from the free list.
    candidateBlock = findFirstFit(pool, size);
    if (candidateBlock == NULL) {
        if (unlockNeeded) {
	    pthread_mutex_unlock(pool->poolMutex);
	}
        return NULL;
    }
    
    // Split the block and re-link the free list.
    candidateBlock = splitBlock(pool, candidateBlock, &size);
    if (candidateBlock == NULL) {
        if (unlockNeeded) {
	    pthread_mutex_lock(pool->poolMutex);
	}
	return NULL;
    }

    // Prep the block for return.
    ((long *)candidateBlock)[0] = (long)pool;
    ((long *)candidateBlock)[1] = size;

    // Unlock the mutex if needed.
    if (pool->poolMutex != NULL) {
        if (0 != pthread_mutex_unlock(pool->poolMutex)) {
	    return NULL;
	}
    }

    // &candidateBlock[2] is the start of the user memory block.
    return &(((long *)candidateBlock)[2]);
}

/**
 * @brief  Free an object that was allocated on variable sized memory pool.
 * @param  addr The address of the object to free.
 * @return 0 on success, -1 on failure.
 */
int mempool_variable_free(void *addr)
{
    MempoolVariable *pool = NULL;
    long size = 0;
    long *originalBlock = (long *)addr;

    if (addr == NULL) {
        return MEMPOOL_FAILURE;
    }

    // Find the pool that this block points to & the size of the block.
    originalBlock -= 2;
    pool = (MempoolVariable *)originalBlock[0];
    size = originalBlock[1];
    if (pool->magic != MEMPOOL_VARIABLE_MAGIC) {
        return MEMPOOL_FAILURE;
    }

    // Lock the mutex if needed.
    if (pool->poolMutex != NULL) {
        if (0 != pthread_mutex_lock(pool->poolMutex)) {
	    return MEMPOOL_FAILURE;
	}
    }

    // Add the block back into the free list.
    insertIntoFreeList(pool, (void *)originalBlock);

    // Unlock the mutex if needed.
    if (pool->poolMutex != NULL) {
        if (0 != pthread_mutex_unlock(pool->poolMutex)) {
	    return MEMPOOL_FAILURE;
	}
    }
 
    return MEMPOOL_SUCCESS;
}

/**
 * @brief  Release all the resources associated with this pool.
 * @param  pool The pool to destroy.
 * @return 0 on success, -1 on failure.
 */
int mempool_destroy_variable_pool(MempoolVariable *pool)
{
    if (pool == NULL || pool->magic != MEMPOOL_VARIABLE_MAGIC || pool->pool == NULL) {
        return MEMPOOL_FAILURE;
    }

    free(pool->pool);
    
    if (pool->poolMutex != NULL) {
        pthread_mutex_destroy(pool->poolMutex);
	free(pool->poolMutex);
    }

    return MEMPOOL_SUCCESS;
}

/**
 * @brief Find the first block in the free list that can satisfy the request.
 * @param pool The pool to search.
 * @param size The size of block to search for.
 * @return The address of the first block that fits, NULL if no fit present.
 */
static void *findFirstFit(MempoolVariable *pool, long size)
{
    void *firstFit = NULL;
    long *blockMetadata = NULL;
    void *currentBlock = pool->freeList;

    while (currentBlock != NULL) {
        blockMetadata = (long *)currentBlock;
	
	// Does this block fit the request?
	if (size <= blockMetadata[VPMD_SIZE_OFFSET]) {
	    firstFit = currentBlock;
	    break;
	}

	currentBlock = (void *)blockMetadata[VPMD_NEXT_OFFSET];
    }

    return firstFit;
}

/**
 * @brief Split the block and put the remainder back into the list.
 * @param pool The pool that this block belongs to.
 * @param addr The address of the block to split.
 * @param requestSize The desired size of block to retrieve.
 * @return The address of the allocated block to return to the caller.
 */
static void *splitBlock(MempoolVariable *pool, void *addr, long *requestSize)
{
    void *allocatedBlock = NULL;
    long *blockMetadata= (long *)addr;
    long blockSize = blockMetadata[VPMD_SIZE_OFFSET];
    long *prevBlock = NULL;
    long *nextBlock = NULL;
    long size = *requestSize;

    // Ensure that the remaining block has space atleast one allocation, 
    // if not, just send it along with allocated data block.
    if ((blockSize - size) < (4 * sizeof(long))) {
        size = blockSize;
    }

    if (blockSize == size) {
        // Unlink the block.
	allocatedBlock = addr;
        prevBlock = (long *)blockMetadata[VPMD_PREV_OFFSET];
        nextBlock = (long *)blockMetadata[VPMD_NEXT_OFFSET];

	if (prevBlock != NULL) {
	    // Current block is not the head of the list.
	    
	    // Point the prev block's next pointer to the current block's next.
	    prevBlock[VPMD_NEXT_OFFSET] = (long)nextBlock;

	    // If the current block is not the tail of the list, make its next
	    // blocks backpointer point its prev block.
	    if (nextBlock != NULL) {
	        nextBlock[VPMD_PREV_OFFSET] = (long)prevBlock;
	    }
	} else {
	    // Current block is the head of the list.

	    // Point the free list head pointer to the next block.
	    pool->freeList = (void *)nextBlock;
	    
	    // If the current block is not the tail of the list null out the prev
	    // pointer of the next block.
	    if (nextBlock != NULL) {
	        nextBlock[VPMD_PREV_OFFSET] = 0;
	    }
	}
    } else {
        // Just reduce the size of the block.
	blockMetadata[VPMD_SIZE_OFFSET] = blockSize - size;
	allocatedBlock = (void *)((long)addr + (blockSize - size));
    }

    // Update the size of the requested block just in case we grew it.
    *requestSize = size;
    return allocatedBlock;
}


/**
 * @brief Insert a block back into the free list, which is ordered by address.
 * @param pool The pool to add the block to.
 * @param addr The address to add into the free list.
 */
static void insertIntoFreeList(MempoolVariable *pool, void *addr)
{
    long *listHead = (long *)pool->freeList;
    long *insertBefore = (long *)pool->freeList;
    long *prev = NULL;
    long *current = (long *)addr;
    long *next = NULL;
    int insertAfterInstead = 0;

    if (listHead == NULL) {
        // Simple case, this block is the new head.
	current[VPMD_NEXT_OFFSET] = 0;
	listHead = current;
    } else {
        // Block needs to be inserted into the list.
	listHead = (listHead > current) ? current : listHead;
	
	// First, find the node that it needs to be inserted before.
	insertBefore = listHead;
        while (insertBefore < current) {
	   // Make sure we dont run off the end of the list.
	   if (insertBefore[VPMD_NEXT_OFFSET] == 0) {
	       insertAfterInstead = 1;
	       break;
	   }
           // Keep traversing otherwise.
	   insertBefore = (long *)(insertBefore[VPMD_NEXT_OFFSET]);
        }

	// Now actually insert the node before or after the node.
	if (insertAfterInstead) {
	    insertAfter(insertBefore, current);
	} else {
	    insertAfter(current, insertBefore);
	}
    }

    // If the head of the list has changed, update the free list pointer.
    if (pool->freeList != listHead) {
       pool->freeList = (void *)listHead;
       listHead[VPMD_PREV_OFFSET] = 0;
    }

    // Finally, call coalesce to minimize fragmentation.
    prev = (long *)current[VPMD_PREV_OFFSET];
    next = (long *)current[VPMD_NEXT_OFFSET];
    coalesceBlocks(prev, current, next);

    return;
}

/**
 * @brief Insert one block after another.
 * @param prev The block to insert after.
 * @param blockToInsert The block to insert.
 */
static void insertAfter(long *prev, long *blockToInsert)
{
    long *next = (long *)prev[VPMD_NEXT_OFFSET];

    prev[VPMD_NEXT_OFFSET] = (long)blockToInsert;
    blockToInsert[VPMD_PREV_OFFSET] = (long)prev;

    blockToInsert[VPMD_NEXT_OFFSET] = (long)next;
    if (next != NULL) {
        next[VPMD_PREV_OFFSET] = (long)blockToInsert;
    }

    return;
}

/**
 * @brief Coalesce the free list into larger blocks.
 * @param prev The previous block in the free list.
 * @param current The block to coalesce.
 * @param next The next block in the free list.
 */
static void coalesceBlocks(long *prev, long *current, long *next)
{
    if (next != NULL) {
        coalesce(current, next); 
    }

    if (prev != NULL) {
        coalesce(prev, current);
    }

    return;
}

/**
 * @brief Coalesce this block with the next one.
 * @param current The block to coalesce.
 * @param next The block to coalesce with.
 */
static void coalesce(long *current, long *next)
{
    unsigned long currentAddr = (long)current;
    unsigned long nextAddr = (long)next;
    long *nextNext = (long *)next[VPMD_NEXT_OFFSET];

    if (currentAddr + current[VPMD_SIZE_OFFSET] == nextAddr) {
        // First, adjust the size.
        current[VPMD_SIZE_OFFSET] += next[VPMD_SIZE_OFFSET];

	// Next, adjust the links.
	current[VPMD_NEXT_OFFSET] = (long)nextNext;
	if (nextNext != NULL) {
	    nextNext[VPMD_PREV_OFFSET] = (long)current;
	}
    }

    return;
}

