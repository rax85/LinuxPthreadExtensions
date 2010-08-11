/**
 * @file   pcQueue.c
 * @brief  An implementation of a producer consumer queue.
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

#include "pcQueue.h"

/**
 * @brief  Initialize a queue. Allocates any resources needed upfront.
 * @param  queue The queue to initialize.
 * @param  queueDepth The maximum number of items allowed in the queue.
 * @return 0 on success, -1 on failure.
 */
int lpx_pcq_init(lpx_pcq_t *queue, int queueDepth);
{
    return PCQ_SUCCESS;
}

/**
 * @brief  Enqueue an item into the queue. Will block until there is space
 *         to enqueue the item.
 * @param  queue The queue to operate on.
 * @param  data  Pointer to the data to insert.
 * @return 0 on success, -1 on failure.
 */
int lpx_pcq_enqueue(lpx_pcq_t *queue, void *data);
{
    return PCQ_SUCCESS;
}

/**
 * @brief  Dequeue an item from the queue. Will block until something is 
 *         available to dequeue.
 * @param  queue The queue to operate on.
 * @param  data  An output pointer to set to the dequeued data.
 * @return 0 on success, -1 on failure.
 */
int lpx_pcq_dequeue(lpx_pcq_t *queue, void **data);
{
    return PCQ_SUCCESS;
}

/**
 * @brief  Enqueue an item into the queue. Will timeout if it cannot 
 *         enqueue an item before the specified timeout.
 * @param  queue The queue to operate on.
 * @param  data  Pointer to the data to insert.
 * @param  timeout Timeout in milliseconds.
 * @return 0 on success, -1 on failure.
 */
int lpx_pcq_timed_enqueue(lpx_pcq_t *queue, void *data, int timeout);
{
    return PCQ_SUCCESS;
}

/**
 * @brief  Dequeue an item from the queue. Will timeout if it cannot
 *         dequeue an item before the specified timeout.
 * @param  queue The queue to operate on.
 * @param  data  An output pointer to set to the dequeued data.
 * @param  timeout Timeout in milliseconds.
 * @return 0 on success, -1 on failure.
 */
int lpx_pcq_timed_dequeue(lpx_pcq_t *queue, void **data, int timeout);
{
    return PCQ_SUCCESS;
}

/**
 * @brief  Destroy a queue and release all associated resources.
 * @param  queue The queue to destroy.
 * @return 0 on success, -1 on failure.
 */
int lpx_pcq_destroy(lpx_pcq_t *queue);
{
    return PCQ_SUCCESS;
}

