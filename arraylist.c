/**
 * @file   arraylist.c
 * @brief  An implementation of an arraylist.
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

#include "arraylist.h"

/**
 * @def ALLOC
 * @brief A wrapper around the pool alloc and regular malloc.
 */
#define ALLOC(pool,size) (((pool) == NULL)? malloc((size)) : lpx_mempool_variable_alloc((pool), (size)))

/**
 * @def FREE
 * @brief A wrapper around the pool free or regular free.
 */
#define FREE(pool,ptr) (((pool) == NULL) ? free((ptr)) : lpx_mempool_variable_free((ptr)))

static int constructArraylist(lpx_arraylist_t *list, int isProtected, lpx_mempool_variable_t *pool);
static int growList(lpx_arraylist_t *list);
static inline long *getElementAtIndex(lpx_arraylist_t *list, long index);

/**
 * @brief  Create an arraylist.
 * @param  list The list to initialize.
 * @param  isProtected ARRAYLIST_PROTECTED if it should be protected by a mutex,
 *                     ARRAYLIST_UNPROTECTED if not.
 * @return 0 on success, -1 on error.
 */
int lpx_arraylist_init(lpx_arraylist_t *list, int isProtected)
{
    if (list == NULL) {
        return ARRAYLIST_ERROR;
    }

    return constructArraylist(list, isProtected, NULL);
}

/**
 * @brief  Create an arraylist.
 * @param  list The list to initialize.
 * @param  isProtected ARRAYLIST_PROTECTED if it should be protected by a mutex,
 *                     ARRAYLIST_UNPROTECTED if not.
 * @param  pool The pool to use for allocations and deallocations.
 * @return 0 on success, -1 on error.
 */
int lpx_arraylist_init_from_pool(lpx_arraylist_t *list, int isProtected, lpx_mempool_variable_t *pool)
{
    if (list == NULL || pool == NULL) {
        return ARRAYLIST_ERROR;
    }

    return constructArraylist(list, isProtected, pool);
}

/**
 * @brief  Create the arraylist.
 * @param  list Pointer to the list that has to be created.
 * @param  isProtected Use an rwlock to protect the list or not?
 * @param  pool The memory pool to use, if any.
 * @return 0 on success, -1 on error.
 */
static int constructArraylist(lpx_arraylist_t *list, int isProtected, lpx_mempool_variable_t *pool)
{
    int i = 0;

    list->pool = pool;

    // Initialize any protection mechanisms requested.
    if (isProtected == ARRAYLIST_PROTECTED) {
        // Allocate space for the rwlock.
        list->rwlock = ALLOC(list->pool, sizeof(lpx_rwlock_t));
	if (list->rwlock == NULL) {
	    return ARRAYLIST_ERROR;
	}
	
	// Initialize the rwlock.
        if (0 != lpx_rwlock_init(list->rwlock)) {
	    goto construct_error1;
	}
    }

    // Allocate the number of heads.
    list->size = 0;
    list->numHeads = ARRAYLIST_DEFAULT_NUMHEADS;
    list->heads = ALLOC(list->pool, ARRAYLIST_DEFAULT_NUMHEADS * sizeof(long *));
    if (list->heads == NULL) {
        goto construct_error2;
    }

    // Initialize all the heads to NULL.
    for (i = 0; i < ARRAYLIST_DEFAULT_NUMHEADS; i++) {
        list->heads[i] = NULL;
    }

    // Now allocate space in the first head.
    list->heads[0] = ALLOC(list->pool, sizeof(long) * ARRAYLIST_HEAD_SIZE);
    if (list->heads[0] == NULL) {
        goto construct_error3;
    }
    
    return ARRAYLIST_SUCCESS;

construct_error3: FREE(list->pool, list->heads);
construct_error2: lpx_rwlock_destroy(list->rwlock);
construct_error1: FREE(list->pool, list->rwlock);
    return ARRAYLIST_ERROR;

}

/**
 * @brief  Destroy the arraylist and release all resources.
 * @param  list The list to work with.
 * @return 0 on success, -1 on error.
 */
int lpx_arraylist_destroy(lpx_arraylist_t *list)
{
    int i = 0;

    if (list == NULL) {
        return ARRAYLIST_ERROR;
    }

    // Destroy each of the parts of the arraylist.
    for (i = 0; i < list->numHeads; i++) {
        if (NULL != list->heads[i]) {
	    FREE(list->pool, list->heads[i]);
	}
    }

    // Destroy the array of heads.
    FREE(list->pool, list->heads);

    // Destroy the reader writer lock.
    if (list->rwlock != NULL) {
        lpx_rwlock_destroy(list->rwlock);
	FREE(list->pool, list->rwlock);
    }

    return ARRAYLIST_SUCCESS;
}

/**
 * @brief  Get the value at the specified index.
 * @param  list The list to work with.
 * @param  index The index to fetch.
 * @param  value Pointer to the location to store the value at.
 * @return 0 on success, -1 on error, -2 if out of bounds.
 */
int lpx_arraylist_get(lpx_arraylist_t *list, long index, long *value)
{
    int retval = ARRAYLIST_SUCCESS;
    long *element = NULL;

    if (list == NULL || value == NULL || index < 0) {
        return ARRAYLIST_ERROR;
    }

    // Acquire the rwlock if necessary.
    if (list->rwlock != NULL && lpx_rwlock_acquire_reader_lock(list->rwlock) != 0) {
        return ARRAYLIST_ERROR;
    }

    if (index >= list->size) {
        // Index is out of bounds.
        retval = ARRAYLIST_OOB;
    } else {
        // Index is within bounds, get the value.
        element = getElementAtIndex(list, index);
	*value = *element;
    }

    // Release the rwlock if necessary.
    if (list->rwlock != NULL && lpx_rwlock_release_reader_lock(list->rwlock) != 0) {
        return ARRAYLIST_ERROR;
    }
    
    return retval;
}

/**
 * @brief  Change the value stored at the specified index to the specified value.
 * @param  list The arraylist to work with.
 * @param  index The index to set.
 * @param  value The value to store.
 * @return 0 on success, -1 on error, -2 if out of bounds.
 */
int lpx_arraylist_set(lpx_arraylist_t *list, long index, long value)
{
    int retval = ARRAYLIST_SUCCESS;
    long *element = NULL;

    if (list == NULL || index < 0) {
        return ARRAYLIST_ERROR;
    }

    // Acquire the rwlock if necessary.
    if (list->rwlock != NULL && lpx_rwlock_acquire_writer_lock(list->rwlock) != 0) {
        return ARRAYLIST_ERROR;
    }

    if (index >= list->size) {
        // Index is out of bounds.
        retval = ARRAYLIST_OOB;
    } else {
        // Index is within bounds, set the value.
        element = getElementAtIndex(list, index);
	*element = value;
    }

    // Release the rwlock if necessary.
    if (list->rwlock != NULL && lpx_rwlock_release_writer_lock(list->rwlock) != 0) {
        return ARRAYLIST_ERROR;
    }
    
    return retval;
}

/**
 * @brief  Add the specified value tot he end of the list.
 * @param  list The arraylist to work with.
 * @param  value The value to append.
 * @return 0 on success -1 on failure.
 */
int lpx_arraylist_append(lpx_arraylist_t *list, long value)
{
    int retval = ARRAYLIST_SUCCESS;
    long *element = NULL;
    long newIndex = 0;

    if (list == NULL) {
        return ARRAYLIST_ERROR;
    }

    // Acquire the rwlock if necessary.
    if (list->rwlock != NULL && lpx_rwlock_acquire_writer_lock(list->rwlock) != 0) {
        return ARRAYLIST_ERROR;
    }

    // Time to append to the end of the list.
    do {
        // If we have no place to insert the element, grow the list.
        if (list->size == list->numHeads * ARRAYLIST_HEAD_SIZE) {
	    if (growList(list) != 0) {
	        retval = ARRAYLIST_ERROR;
		break;
	    }
	}

	// See if we need to add a head.
	newIndex = list->size / ARRAYLIST_HEAD_SIZE;
	if (list->heads[newIndex] == NULL) {
	    list->heads[newIndex] = ALLOC(list->pool, sizeof(long) * ARRAYLIST_HEAD_SIZE);
	    if (list->heads[newIndex] == NULL) {
	        return ARRAYLIST_ERROR;
	    }
	}

	// Set the element at the end of the list and increase the size.
	element = getElementAtIndex(list, list->size);
	*element = value;
        list->size++;
    } while (0);


    // Release the rwlock if necessary.
    if (list->rwlock != NULL && lpx_rwlock_release_writer_lock(list->rwlock) != 0) {
        return ARRAYLIST_ERROR;
    }

    return retval;
}

/**
 * @brief  Grow the arraylist to make place for the next head.
 * @param  list The list to grow.
 * @return 0 on success -1 on failure.
 */
static int growList(lpx_arraylist_t *list)
{
    long **newHeads = NULL;

    // Grow the list of heads if we have run out of slots in the array of heads.
    if (list->heads[list->numHeads - 1] != NULL) {
        newHeads = ALLOC(list->pool, (sizeof(long *) * list->numHeads * 2));
	if (newHeads == NULL) {
	    return ARRAYLIST_ERROR;
	}

	memset(newHeads, 0, sizeof(long *) * list->numHeads * 2);
	memcpy(newHeads, list->heads, sizeof(long *) * list->numHeads);
	FREE(list->pool, list->heads);
	list->heads = newHeads;
	list->numHeads *= 2;
    }

    return ARRAYLIST_SUCCESS;
}

/**
 * @brief  Remove the element at the specified index from the list. This will
 *         shift all the elements left by one, so it is a reasonably expensive
 *         operation for large lists. Use with caution.
 * @param  list The list to work with.
 * @param  index The index to remove.
 * @return 0 on success, -1 on failure, -2 on out of bounds.
 */
int lpx_arraylist_remove(lpx_arraylist_t *list, long index)
{
    int retval = ARRAYLIST_SUCCESS;
    long *element = NULL;
    long i = 0;

    if (list == NULL || index < 0) {
        return ARRAYLIST_ERROR;
    }

    // Acquire the rwlock if necessary.
    if (list->rwlock != NULL && lpx_rwlock_acquire_writer_lock(list->rwlock) != 0) {
        return ARRAYLIST_ERROR;
    }

    if (index >= list->size) {
        // The specified index to remove was out of bounds.
        retval = ARRAYLIST_OOB;
    } else {
        // Remove the element by making the Nth element equal to the N+1th element.
        for (i = index + 1; i < list->size; i++) {
	    element = getElementAtIndex(list, i - 1);
	    *element = *(getElementAtIndex(list, i));
	}

	list->size--;
    }

    // Release the rwlock if necessary.
    if (list->rwlock != NULL && lpx_rwlock_release_writer_lock(list->rwlock) != 0) {
        return ARRAYLIST_ERROR;
    }

    return retval;
}

/**
 * @brief  Clear all the elements from the arraylist.
 * @param  list The list to clear.
 * @return 0 on success, -1 on failure.
 */
int lpx_arraylist_clear(lpx_arraylist_t *list)
{
    int i = 0;

    if (list == NULL) {
        return ARRAYLIST_ERROR;
    }

    // Acquire the rwlock if necessary.
    if (list->rwlock != NULL && lpx_rwlock_acquire_writer_lock(list->rwlock) != 0) {
        return ARRAYLIST_ERROR;
    }

    // Destroy each of the parts of the arraylist starting from 1 but retain the list
    // of heads because it is expensive to grow the list of heads.
    for (i = 1; i < list->numHeads; i++) {
        if (NULL != list->heads[i]) {
	    FREE(list->pool, list->heads[i]);
	    list->heads[i] = NULL;
	}
    }

    // Reset the size. Its ok if the values remain, the get method will prevent them 
    // from being accessed.
    list->size = 0;

    // Release the rwlock if necessary.
    if (list->rwlock != NULL && lpx_rwlock_release_writer_lock(list->rwlock) != 0) {
        return ARRAYLIST_ERROR;
    }

    return ARRAYLIST_SUCCESS;
}

/**
 * @brief  Return a regular C array that contains all the values in the arraylist
 *         in the same order as they are in the arraylist.
 * @param  list The list to work with.
 * @return The contents of the list as an array on success, NULL on failure.
 */
long *lpx_arraylist_to_array(lpx_arraylist_t *list)
{
    int i = 0;
    long *array = NULL;
    int elementsToCopy = ARRAYLIST_HEAD_SIZE;

    if (list == NULL) {
        return NULL;
    }

    // Acquire the rwlock if necessary.
    if (list->rwlock != NULL && lpx_rwlock_acquire_reader_lock(list->rwlock) != 0) {
        return NULL;
    }

    array = ALLOC(list->pool, list->size * sizeof(long));
    if (array != NULL) {
        for (i = 0; i < list->size; i += ARRAYLIST_HEAD_SIZE) {
	    // Determine how many bytes are remaining in the copy.
	    if ((list->size - i) < elementsToCopy) {
	        elementsToCopy = list->size - i;
	    }

	    memcpy(array + i, list->heads[i / ARRAYLIST_HEAD_SIZE], elementsToCopy * sizeof(long));
	}
    }

    // Release the rwlock if necessary.
    if (list->rwlock != NULL && lpx_rwlock_release_reader_lock(list->rwlock) != 0) {
        return NULL;
    }

    return array;
}

/**
 * @brief  Get the number of elements in the list.
 * @param  list The list to check.
 * @return The size of the list on success, -1 on error.
 */
long lpx_arraylist_size(lpx_arraylist_t *list)
{
    long retval = -1;
    
    if (list == NULL) {
        return ARRAYLIST_ERROR;
    }
    
    // Acquire the rwlock if necessary.
    if (list->rwlock != NULL && lpx_rwlock_acquire_reader_lock(list->rwlock) != 0) {
        return ARRAYLIST_ERROR;
    }

    retval = list->size;

    // Release the rwlock if necessary.
    if (list->rwlock != NULL && lpx_rwlock_release_reader_lock(list->rwlock) != 0) {
        return ARRAYLIST_ERROR;
    }

    return retval;
}

/**
 * @brief  Get a pointer to the element at the given index.
 * @param  list The list to check.
 * @param  index The index of the element to fetch.
 * @return A pointer to the element at the given index.
 */
static inline long *getElementAtIndex(lpx_arraylist_t *list, long index)
{
    long *element = NULL;
    int headNum = index / ARRAYLIST_HEAD_SIZE;
    int elementNum = index % ARRAYLIST_HEAD_SIZE;

    long *row = list->heads[headNum];
    element = &(row[elementNum]);

    return element;
}

/**
 * @brief  Search the list for a key. 
 * @param  list The list to search.
 * @param  key The search key.
 * @return The index of the element if found, -1 otherwise.
 */
long lpx_arraylist_get_index(lpx_arraylist_t *list, long key)
{
    long i = 0;
    int j = 0;
    long index = -1;
    long *row = NULL;

    if (list == NULL) {
        return ARRAYLIST_ERROR;
    }

    // Acquire the rwlock if necessary.
    if (list->rwlock != NULL && lpx_rwlock_acquire_reader_lock(list->rwlock) != 0) {
        return ARRAYLIST_ERROR;
    }

    for (i = 0; i < list->numHeads; i++) {
        if (list->heads[i] == NULL) {
	    break;
	}

        row = list->heads[i];
	for (j = 0; j < ARRAYLIST_HEAD_SIZE; j++) {
	    if (row[j] == key) {
	        index = i * ARRAYLIST_HEAD_SIZE + j;
		break;
	    }
	}

	// Break if we found the key.
	if (index != -1) {
	    break;
	}
    }

    // Release the rwlock if necessary.
    if (list->rwlock != NULL && lpx_rwlock_release_reader_lock(list->rwlock) != 0) {
        return ARRAYLIST_ERROR;
    }

    return index;
}

