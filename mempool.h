/**
 * @file   mempool.h
 * @brief  Memory pools to segregate the heap. The intention of this is to have
 *         different heaps for different threads. This solves a few use cases, if
 *         each thread uses its own heap, it can use the non thread safe versions of 
 *         the allocators. Even if pools are shared, it still reduces contention for
 *         the global heap of the program. Also, releasing the entire pool at thread
 *         exit allows for any leaked memory to be reclaimed.
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

#ifndef __MEMPOOL_H__
#define __MEMPOOL_H__

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

/**
 * @def MEMPOOL_SUCCESS
 * @brief Denotes that the mempool operation succeeded.
 */
#define MEMPOOL_SUCCESS			0	
/**
 * @def MEMPOOL_FAILURE
 * @brief Denotes that the mempool operation failed.
 */
#define MEMPOOL_FAILURE			-1

/**
 * @def MEMPOOL_PROTECTED
 * @brief Specifies that the pool should be thread safe.
 */
#define MEMPOOL_PROTECTED		2
/**
 * @def MEMPOOL_UNPROTECTED
 * @brief Specifies that the thread pool need not be thread safe.
 */
#define MEMPOOL_UNPROTECTED		3	

/**
 * @def   MEMPOOL_FIXED_MAGIC
 * @brief Magic number to verify the integrity of a fixed pool.
 */
#define MEMPOOL_FIXED_MAGIC		0xdecaf123
/**
 * @def   MEMPOOL_VARIABLE_MAGIC
 * @brief Magic number to verify the integrity of a variable pool.
 */
#define MEMPOOL_VARIABLE_MAGIC		0xc0ffee12

/**
 * @def   MEMPOOL_PER_OBJECT_OVERHEAD
 * @brief How much memory is needed for metadata for each object in a fixed pool.
 */
#define MEMPOOL_PER_OBJECT_OVERHEAD	(sizeof (void *))

/**
 * @def   MEMPOOL_PER_BLOCK_OVERHEAD	
 * @brief How much memory is needed for metadata for each block in a variable pool.
 */
#define MEMPOOL_PER_BLOCK_OVERHEAD	(2 * sizeof(long))

/**
 * @def   VPMD_SIZE_OFFSET 
 * @brief Offset of the size in the free list block.
 */
#define VPMD_SIZE_OFFSET	0

/**
 * @def   VPMD_PREV_OFFSET
 * @brief Offset of the previous pointer in the free list block.
 */
#define VPMD_PREV_OFFSET	1

/**
 * @def   VPMD_NEXT_OFFSET
 * @brief Index of the next pointer in the free list block.
 */
#define VPMD_NEXT_OFFSET	2

/**
 * @brief A struct to represent a memory pool of fixed sized objects.
 */
typedef struct __MempoolFixed {
    pthread_mutex_t *poolMutex;		/**< A mutex to protect the pool if needed. */
    void *pool;				/**< The actual memory pool. */
    void *freeList;			/**< List of free nodes. */
    long poolSize;			/**< Size of the actual memory pool. */
    long storedObjectSize;		/**< Object size + overhead */
    int magic;				/**< Enables a simple integrity check. */
}MempoolFixed;


/*
 * Structure of the free list.
 *                                 |----------------------------+
 *             +---------------+   +-----+----+----+    +-----+-+--+----+
 * freeList--->|blkSz|NULL|next+-->|blkSz|prev|next+--> |blkSz|prev|NULL|
 *             +-----+----+----+   +-----+-+--+----+    +-----+----+----+
 *             |---------------------------+
 *
 * > Each node is sizeof(long) + sizeof(long) + sizeof(long).
 */

/**
 * @brief A struct to represent a memory pool of variable sized objects.
 */
typedef struct __MempoolVariable {
    pthread_mutex_t *poolMutex;		/**< A mutex to protect the pool if needed. */
    void *pool;				/**< The actual memory pool. */
    long poolSize;			/**< The size of the actual pool. */
    void *freeList;			/**< List of free blocks. */
    int magic;				/**< Enables a simple integrity check. */
}MempoolVariable;

int mempool_create_fixed_pool(MempoolFixed *, long, int, int);
void *mempool_fixed_alloc(MempoolFixed *);
int mempool_fixed_free(void *);
int mempool_destroy_fixed_pool(MempoolFixed *);

int mempool_create_variable_pool(MempoolVariable *, long, int);
void *mempool_variable_alloc(MempoolVariable *, long);
int mempool_variable_free(void *);
int mempool_destroy_variable_pool(MempoolVariable *);

static void *findFirstFit(MempoolVariable *, long);
static void *splitBlock(MempoolVariable *, void *, long *);
static void insertIntoFreeList(MempoolVariable *, void *);
static void insertAfter(long *, long *);
static void insertBefore(long *, long *);
static void coalesceBlocks(long *, long *, long *);
static void coalesce(long *, long *);

#endif
