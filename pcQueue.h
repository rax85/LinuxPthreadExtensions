/**
 * @file   
 * @brief  
 * @author Rakesh Iyer.
 * @bug    Not tested.
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

#ifndef __PCQUEUE_H
#define __PCQUEUE_H

#include "mempool.h"


/**
 * @def PCQ_SUCCESS
 * @brief Indicates that the queue operation succeeded.
 */
#define PCQ_SUCCESS		0

/**
 * @def PCQ_FAILURE
 * @brief Indicates that the queue operation failed.
 */
#define PCQ_FAILURE		-1

/**
 * @def PCQ_TIMEOUT
 * @brief Indicates that the queue operation timed out.
 */
#define PCQ_TIMEOUT		-2

/**
 * @brief Defines a single node in the producer consumer queue.
 */
typedef struct __lpx_pcq_node_t {
    void *data;				/**< A pointer to the data. */
    struct __lpx_pcq_node_t *next;	/**< Next data node. NULL if tail node. */
    struct __lpx_pcq_node_t *prev;	/**< Previous data node. NULL if head node. */
}lpx_pcq_node_t;

/**
 * @brief Defines a producer consumer queue of a fixed max size.
 */
typedef struct __lpx_pcq_t {
    int maxSize;		/**< The maximum number of items in the queue. */
    lpx_mempool_fixed pool;	/**< Pool for storing the nodes. */
    lpx_semaphore_t sem;	/**< Semaphore to keep count of available data. */
    pthread_mutex_t qMutex;	/**< Mutex to protect the queue. */
    lpx_pcq_node_t *head;	/**< Pointer to the head of the queue. */
    lpx_pcq_node_t *tail;	/**< Pointer to teh tail of the queue. */
} lpx_pcq_t;

int lpx_pcq_init(lpx_pcq_t *queue, int queueDepth);
int lpx_pcq_enqueue(lpx_pcq_t *queue, void *data);
int lpx_pcq_dequeue(lpx_pcq_t *queue, void **data);
int lpx_pcq_timed_enqueue(lpx_pcq_t *queue, void *data, int timeout);
int lpx_pcq_timed_dequeue(lpx_pcq_t *queue, void **data, int timeout);
int lpx_pcq_destroy(lpx_pcq_t *queue);

#endif

