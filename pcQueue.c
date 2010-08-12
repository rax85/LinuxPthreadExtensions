/**
 * @file   pcQueue.c
 * @brief  An implementation of a producer consumer queue.
 * @author Rakesh Iyer.
 * @bug    Not performance tested.
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

// Forward declarations of functions used internally.
static int enqueue(lpx_pcq_t *queue, void *value);
static int dequeue(lpx_pcq_t *queue, void **value);

/**
 * @brief  Initialize a queue. Allocates any resources needed upfront.
 * @param  queue The queue to initialize.
 * @param  queueDepth The maximum number of items allowed in the queue.
 * @return 0 on success, -1 on failure.
 */
int lpx_pcq_init(lpx_pcq_t *queue, int queueDepth)
{
    if (queue == NULL || queueDepth < 1) {
        return PCQ_FAILURE;
    }

    queue->maxSize = queueDepth;

    // The nqSem is decremented every time someone wants to put something on the
    // queue so it should be initially available, which is the default.
    if (0 != lpx_sem_init(&queue->nqSem, queueDepth)) {
        return PCQ_FAILURE;
    }

    // The dqSem is decremented every time someone wants to pull something off the
    // queue so it should be initially completely unavailable.
    if (0 != lpx_sem_init(&queue->dqSem, queueDepth)) {
        goto pcq_destroy1;
    }
 
    if (0 != lpx_sem_down_multiple(&queue->dqSem, queueDepth)) {
        goto pcq_destroy2;
    }

    // The mutex is for the queue itself.
    if (0 != pthread_mutex_init(&queue->qMutex, NULL)) {
        goto pcq_destroy2;
    }

    // Use a mempool to hold the nodes of the list instead of using malloc.
    if (0 != lpx_mempool_create_fixed_pool(&queue->pool, sizeof(lpx_pcq_node_t),
                                           queueDepth, MEMPOOL_PROTECTED)) {
        goto pcq_destroy3;
    }

    queue->head = NULL;
    queue->tail = NULL;

    return PCQ_SUCCESS;

pcq_destroy3: pthread_mutex_destroy(&queue->qMutex);
pcq_destroy2: lpx_sem_destroy(&queue->dqSem);
pcq_destroy1: lpx_sem_destroy(&queue->nqSem);
    return PCQ_FAILURE;
}

/**
 * @brief  Enqueue an item into the queue. Will block until there is space
 *         to enqueue the item.
 * @param  queue The queue to operate on.
 * @param  data  Pointer to the data to insert.
 * @return 0 on success, -1 on failure.
 */
int lpx_pcq_enqueue(lpx_pcq_t *queue, void *data)
{
    if (queue == NULL) {
        return PCQ_FAILURE;
    }

    // Decrement the nqSem first to ensure there is space available.
    if (0 != lpx_sem_down(&queue->nqSem)) {
        return PCQ_FAILURE;
    }
    
    // Add the node to the queue.
    if (0 != enqueue(queue, data)) {
        lpx_sem_up(&queue->nqSem);
	return PCQ_FAILURE;
    }
    
    // Increment the dqSem to allow others to dequeue.
    if (0 != lpx_sem_up(&queue->dqSem)) {
        return PCQ_FAILURE;
    }

    return PCQ_SUCCESS;
}

/**
 * @brief  Dequeue an item from the queue. Will block until something is 
 *         available to dequeue.
 * @param  queue The queue to operate on.
 * @param  data  An output pointer to set to the dequeued data.
 * @return 0 on success, -1 on failure.
 */
int lpx_pcq_dequeue(lpx_pcq_t *queue, void **data)
{
    if (queue == NULL || data == NULL) {
        return PCQ_FAILURE;
    }

    // Decrement the dqSem to ensure there is something to dequeue.
    if (0 != lpx_sem_down(&queue->dqSem)) {
        return PCQ_FAILURE;
    }
    
    // Actually dequeue the data.
    if (0 != dequeue(queue, data)) {
        lpx_sem_up(&queue->dqSem);
	return PCQ_FAILURE;
    }
    
    // Increment the nqSem to allow others to enqueue.
    if (0 != lpx_sem_up(&queue->nqSem)) {
        return PCQ_FAILURE;
    }

    return PCQ_SUCCESS;
}

/**
 * @brief  Enqueue an item into the queue. Will timeout if it cannot get a chance
 *         to enqueue before the timeout. After that the timeout is kind of loose.
 *         For an explanation, see lpx_pcq_timed_dequeue. Restassured, if it 
 *         doesn't timeout, it will not block infinitely.
 * @param  queue The queue to operate on.
 * @param  data  Pointer to the data to insert.
 * @param  timeout Timeout in milliseconds.
 * @return 0 on success, -1 on failure.
 */
int lpx_pcq_timed_enqueue(lpx_pcq_t *queue, void *data, long timeout)
{
    int retval = 0;

    if (queue == NULL) {
        return PCQ_FAILURE;
    }

    // Decrement the nqSem first to ensure there is space available.
    if ((retval = lpx_sem_timed_down(&queue->nqSem, 1, timeout)) != 0) {
        if (retval == SEMAPHORE_TIMEOUT) {
	    return PCQ_TIMEOUT;
	} else {
	    return PCQ_FAILURE;
	}
    }
    
    // Add the node to the queue.
    if (0 != enqueue(queue, data)) {
        lpx_sem_up(&queue->nqSem);
	return PCQ_FAILURE;
    }
    
    // Increment the dqSem to allow others to dequeue.
    if (0 != lpx_sem_up(&queue->dqSem)) {
        return PCQ_FAILURE;
    }

    return PCQ_SUCCESS;
}

/**
 * @brief  Dequeue an item from the queue. Will timeout if it cannot get a 
 *         chance to dequeue before the timeout. However the timeout is kind of
 *         loose because after it gets the chance to dequeue, no more timeouts are
 *         currently enforced. Luckily none of the other locks can be infinitely 
 *         held so if it doesn't timeout, it will always succeed.
 * @param  queue The queue to operate on.
 * @param  data  An output pointer to set to the dequeued data.
 * @param  timeout Timeout in milliseconds.
 * @return 0 on success, -1 on failure.
 */
int lpx_pcq_timed_dequeue(lpx_pcq_t *queue, void **data, long timeout)
{
    int retval = 0;

    if (queue == NULL || data == NULL) {
        return PCQ_FAILURE;
    }

    // Decrement the dqSem to ensure there is something to dequeue.
    if ((retval = lpx_sem_timed_down(&queue->dqSem, 1, timeout)) != 0) {
        if (retval == SEMAPHORE_TIMEOUT) {
	    return PCQ_TIMEOUT;
	} else {
	    return PCQ_FAILURE;
	}
    }
    
    // Actually dequeue the data.
    if (0 != dequeue(queue, data)) {
        lpx_sem_up(&queue->dqSem);
	return PCQ_FAILURE;
    }
    
    // Increment the nqSem to allow others to enqueue.
    if (0 != lpx_sem_up(&queue->nqSem)) {
        return PCQ_FAILURE;
    }

    return PCQ_SUCCESS;
}

/**
 * @brief  Destroy a queue and release all associated resources.
 * @param  queue The queue to destroy.
 * @return 0 on success, -1 on failure.
 */
int lpx_pcq_destroy(lpx_pcq_t *queue)
{
    lpx_mempool_destroy_fixed_pool(&queue->pool);
    lpx_sem_destroy(&queue->nqSem);
    lpx_sem_destroy(&queue->dqSem);
    pthread_mutex_destroy(&queue->qMutex);
    return PCQ_SUCCESS;
}

/**
 * @brief  Add a value to the queue.
 * @param  queue The queue to enqueue into.
 * @param  value The value to enqueue.
 * @return 0 on success, -1 on failure.
 */
static int enqueue(lpx_pcq_t *queue, void *value)
{
    // Get some memory for the node.
    lpx_pcq_node_t *node = lpx_mempool_fixed_alloc(&queue->pool);
    
    if (node == NULL) {
        // Should never happen in good circumstances.
        return PCQ_FAILURE;
    }

    // Store the data in the node.
    node->data = value;
    node->next = NULL;

    // Lock the queue before operating on it.
    if (0 != pthread_mutex_lock(&queue->qMutex)) {
        lpx_mempool_fixed_free(node);
        return PCQ_FAILURE;
    }

    node->prev = queue->tail;
    if (queue->tail != NULL) {
        // Queue is not empty.
        queue->tail->next = node;
        queue->tail = node;
    } else {
        // Queue is empty.
        queue->head = node;
        queue->tail = node;
    }

    // Finally, unlock the queue.
    if (0 != pthread_mutex_unlock(&queue->qMutex)) {
       // This is really fatal.
       return PCQ_FAILURE;
    }
    
    return PCQ_SUCCESS;
}

/**
 * @brief  Dequeue an item from the queue.
 * @param  queue The queue to operate on.
 * @param  value A pointer to where to store the output data.
 * @return 0 on success, -1 on failure.
 */
static int dequeue(lpx_pcq_t *queue, void **value)
{
    lpx_pcq_node_t *node = NULL;
    
    // Grab the queue mutex before manipulating the queue.
    if (0 != pthread_mutex_lock(&queue->qMutex)) {
        return PCQ_FAILURE;
    }

    if (queue->head == NULL) {
        // Fatal. Unlock and return failure.
        pthread_mutex_unlock(&queue->qMutex);
        return PCQ_FAILURE;
    }

    node = queue->head;
    queue->head = queue->head->next;
    if (queue->head != NULL) {
        queue->head->prev = NULL;
    }

    // Unlock the queue mutex
    if (0 != pthread_mutex_unlock(&queue->qMutex)) {
        // The is fatal. There isn't much point in continuing from here.
        return PCQ_FAILURE;
    }
    
    *value = node->data;
    // Free the node we just dequeued.
    lpx_mempool_fixed_free(node);

    return PCQ_SUCCESS;
}

