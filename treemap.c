/**
 * @file   treemap.c
 * @author Rakesh Iyer
 * @brief  An implementation of a map based on a red black tree. For an
 *         explanation of how this worked go to:
 *         http://en.wikipedia.org/wiki/Red-black_tree
 *
 *         The implementation is non recursive... just because...
 *
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

#include "treemap.h"

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

#define PRINT_PATH	0

static int init(lpx_treemap_t *treemap, int isProtected, lpx_mempool_variable_t *pool);
static rbnode *newNode(lpx_treemap_t *treemap, long key, long value);
static inline int isRed(rbnode *node);
static inline int isBlack(rbnode *node);
static int insert(lpx_treemap_t *treemap, long key, long value);
static void resolve_rb_conflics(lpx_treemap_t *treemap, rbnode *node);
static inline rbnode *grandparent(rbnode *node);
static inline rbnode *uncle(rbnode *node);
static inline int isLeftChild(rbnode *parent, rbnode *child);
static inline int isRightChild(rbnode *parent, rbnode *child);
static void rotateLeft(lpx_treemap_t *treemap, rbnode *node);
static void rotateRight(lpx_treemap_t *treemap, rbnode *node);
static int delete(lpx_treemap_t *treemap, rbnode *node);

/**
 * @brief Initialize the treemap.
 * @param treemap The treemap to initialize.
 * @param isProtected Should a rwlock be created for this treemap?
 * @param pool The pool to use, null if there is none.
 * @return 0 on success, -1 on failure.
 */
static int init(lpx_treemap_t *treemap, int isProtected, lpx_mempool_variable_t *pool)
{
    if (treemap == NULL) {
        return TREEMAP_ERROR;
    }

    treemap->pool = pool;

    // Set up the reader writer lock if it is protected.
    if (isProtected == TREEMAP_PROTECTED) {
        treemap->rwlock = ALLOC(pool, sizeof(lpx_rwlock_t));
	if (treemap->rwlock == NULL) {
	    return TREEMAP_ERROR;
	}

        if (0 != lpx_rwlock_init(treemap->rwlock)) {
	    FREE(treemap->pool, treemap->rwlock);
	    return TREEMAP_ERROR;
	}
    } else {
        treemap->rwlock = NULL;
    }

    treemap->head = NULL;

    return TREEMAP_SUCCESS;
}

/**
 * @brief  Initialize the treemap and allocate any needed resources. 
 * @param  treemap The treemap to create.
 * @param  isProtected Should this treemap be thread safe?
 * @return 0 on success, -1 on failure.
 */
int lpx_treemap_init(lpx_treemap_t *treemap, int isProtected)
{
    return init(treemap, isProtected, NULL);
}

/**
 * @brief  Initialize the treemap, except this time use a memory pool instead.
 * @param  treemap The treemap to initalize.
 * @param  isProtected Should this treemap be protected?
 * @param  pool The memory pool to use.
 * @return 0 on success, -1 on failure.
 */
int lpx_treemap_init_from_pool(lpx_treemap_t *treemap, int isProtected, 
                               lpx_mempool_variable_t *pool)
{
    return init(treemap, isProtected, pool);
}

/**
 * @brief Generate a new node for the tree.
 * @param treemap The treemap to generate a node for.
 * @param key The key in the map.
 * @param value The value held in the node.
 * @return A node on success, NULL on failure.
 */
static rbnode *newNode(lpx_treemap_t *treemap, long key, long value)
{
    rbnode *node = ALLOC(treemap->pool, sizeof(rbnode));
    if (node != NULL) {
        node->color = COLOR_RED;
	node->left = NULL;
	node->right = NULL;
	node->parent = NULL;
	node->key = key;
	node->value = value;
    }

    return node;
}

/**
 * @brief Check whether a node is red or not.
 * @param node The node to check.
 * @return 1 if red, 0 if not.
 */
static inline int isRed(rbnode *node)
{
    if (node == NULL) {
        return 0;
    }

    return (node->color == COLOR_RED);
}

/**
 * @brief Check whether a node is black or not.
 * @param node The node to check.
 * @return 1 if black, 0 if not.
 */
static inline int isBlack(rbnode *node)
{
    return !isRed(node);
}

/**
 * @brief Insert the value into the tree.
 * @param treemap The treemap to insert into.
 * @param key The key in the map.
 * @param value The value to insert.
 * @return 0 on success, -1 on failure.
 */
static int insert(lpx_treemap_t *treemap, long key, long value)
{
    // First of all, get a node.
    rbnode *node = newNode(treemap, key, value);
    if (node == NULL) {
        return TREEMAP_ERROR;
    }
    
    // Now insert it simple binary tree style.
    
    // The head node is the easy case.
    if (treemap->head == NULL) {
        treemap->head = node;
        resolve_rb_conflics(treemap, node);
	return TREEMAP_SUCCESS;
    }

    // Ok we have to actually insert it into the tree :)
    rbnode *currentNode = treemap->head;
    while (currentNode != NULL) {
	// Go right?
        if (key > currentNode->key) {
	    if (currentNode->right == NULL) {
	        currentNode->right = node;
		node->parent = currentNode;
		break;
	    } else {
	        currentNode = currentNode->right;
		continue;
	    }

	}
       
	// Go left?
        if (key < currentNode->key) {
	    if (currentNode->left == NULL) {
	        currentNode->left = node;
		node->parent = currentNode;
		break;
	    } else {
	        currentNode = currentNode->left;
		continue;
	    }
	}

	// Replace the current value, deallocate the new node & exit.
	if (currentNode->key == key) {
	    currentNode->value = value;
	    FREE(treemap->pool, node);
	    return TREEMAP_SUCCESS;
	}
    }
    
    // Resolve any red black tree conflicts.
    resolve_rb_conflics(treemap, node);

    // Finally, if the root node ends up red, we can make it black without
    // upsetting the black count to any node.
    treemap->head->color = COLOR_BLACK;

    return TREEMAP_SUCCESS;
}

/**
 * @brief Get the parent of the parent of the node.
 * @param node The node to check.
 * @return The grandparent if it exists, or NULL if not.
 */
static inline rbnode *grandparent(rbnode *node)
{
    rbnode *gpnode = NULL;
    if (node->parent != NULL) {
        gpnode = node->parent->parent;
    }

    return gpnode;
}

/**
 * @brief Check whether the specified node is the left child of the parent.
 * @param parent The parent node to check.
 * @param child The child node to check.
 * @return 1 if it is the left child, 0 if not.
 */
static inline int isLeftChild(rbnode *parent, rbnode *child)
{
    if (parent == NULL || child == NULL) {
       return 0;
    }

    return (parent->left == child);
}

/**
 * @brief Check whether the specified node is the right child of the parent.
 * @param parent The parent node to check.
 * @param child The child node to check.
 * @return 1 if it is the right child, 0 if not.
 */
static inline int isRightChild(rbnode *parent, rbnode *child)
{
    return !isLeftChild(parent, child);
}

/**
 * @brief Get the sibling of the parent of the node.
 * @param node The node to check.
 * @return The uncle if it exists, NULL if not.
 */
static inline rbnode *uncle(rbnode *node)
{
    rbnode *unode = NULL;
    rbnode *gparent = grandparent(node);
    if (gparent != NULL) {
        if (isLeftChild(gparent, node->parent)) {
	    unode = gparent->right;
	} else {
	    unode = gparent->left;
	}
    }

    return unode;
}

/**
 *
 * @brief Resolve any red black tree conflicts after the newly added node.
 * @param treemap The treemap to check for conflicts.
 * @param node The node that we just inserted.
 */
static void resolve_rb_conflics(lpx_treemap_t *treemap, rbnode *node)
{
    rbnode *pnode = NULL;
    rbnode *gnode = NULL;
    rbnode *unode = NULL;
    rbnode *currentNode = node;
    
    while (currentNode != NULL) {
        // The root is always black and a tree with one node is always balanced.
        // If the current node is the parent, repaint it black and get done.
        if (currentNode->parent == NULL) {
            currentNode->color = COLOR_BLACK;
            return;
        }

        // If the new node has a black parent, the tree is already balanced.
        if (isBlack(currentNode->parent)) {
            return;
        }

        /*
	 * Handle cases like this.
	 *                    b
	 *                   / \
	 *                  b   b         <- Pull the black down from here
	 *                       \
	 *                        r       <- To here
	 *
         * If the uncle and parent are both red, repaint them as black and
	 * re-iterate using the grandparent.
	 */
        unode = uncle(currentNode);
	if (isRed(currentNode->parent) && isRed(unode)) {
            // Pull the black down from grandparent.
	    currentNode->parent->color = COLOR_BLACK;
	    unode->color = COLOR_BLACK;

            // Make the grandparent and the current node.
            currentNode = grandparent(currentNode);
            currentNode->color = COLOR_RED;
	} else {
            break;
        } 
    }

    // This point we know that the parent is red and the uncle is black.
    unode = uncle(currentNode);
    pnode = currentNode->parent;
    gnode = grandparent(currentNode);

    /* 
     * Handle situations like this.
     *             o
     *              \
     *               o
     *              / 
     *             o
     *            /
     *           o
     */
    if (isRightChild(pnode, currentNode) && isLeftChild(gnode, pnode)) {
        rotateLeft(treemap, pnode);
        currentNode = currentNode->left;
    } else if (isLeftChild(pnode, currentNode) && isRightChild(gnode, pnode)) {
        rotateRight(treemap, pnode);
        currentNode = currentNode->right;
    }

    /* 
     * Handle situations like this:
     *             o
     *            / \
     *           o   o
     *                \
     *                 o
     *                  \
     *                   o
     */
    gnode = grandparent(currentNode);
    pnode = currentNode->parent;

    pnode->color = COLOR_BLACK;
    gnode->color = COLOR_RED;
    if (isLeftChild(pnode, currentNode) && isLeftChild(gnode, pnode)) {
        rotateRight(treemap, gnode);
    } else {
        rotateLeft(treemap, gnode);
    }
}

/**
 * @brief Rotate the given subtree left by one node. We assume that there
 *        is both a left and right child to this node. We wont call the
 *        method otherwise.
 * @param treemap The treemap to which this node belongs.
 * @param node The node that represents the root of the subtree to rotate.
 */
static void rotateLeft(lpx_treemap_t *treemap, rbnode *node)
{
    rbnode *parent = node->parent;
    rbnode *root = NULL;
    rbnode *rightChild = node->right;

    // For a left rotation first make the right child the root node.
    root = rightChild;
    root->parent = parent;

    if (parent == NULL) {
        // This is a head node.
	treemap->head = root;
    } else if (isLeftChild(parent, node)) {
        // Left child.
	parent->left = root;
    } else {
        // Right child.
	parent->right = root;
    }

    // Move the left child of the old right child to the right child of the old root.
    node->right = root->left;
    if (node->right != NULL) {
        node->right->parent = node;
    }
    
    // Make the old root the left child.
    root->left = node;
    node->parent = root;
}

/**
 * @brief Rotate the given subtree left by one node.
 * @param treemap The treemap to which this node belongs.
 * @param node The node that represents the root of the subtree to rotate.
 */
static void rotateRight(lpx_treemap_t *treemap, rbnode *node)
{
    rbnode *parent = node->parent;
    rbnode *root = NULL;
    rbnode *leftChild = node->left;
  
    // For a right rotation first make the left child the root node.
    root = leftChild;
    root->parent = parent;

    if (parent == NULL) {
        // This is a head node.
	treemap->head = root;
    } else if (isLeftChild(parent, node)) {
        // Left child.
	parent->left = root;
    } else {
        // Right child.
	parent->right = root;
    }

    // Move the right child of the old left child to the left child of the old root.
    node->left = root->right;
    if (node->left != NULL) {
        node->left->parent = node;
    }
    
    // Make the old root the right child.
    root->right = node;
    node->parent = root;
}

/**
 * @brief  Add a key value pair into the tree map.
 * @param  treemap The treemap to operate on.
 * @param  key The key associated with the value.
 * @param  value The value to be added.
 * @return 0 on success, -1 on failure.
 */
int lpx_treemap_put(lpx_treemap_t *treemap, unsigned long key, unsigned long value)
{
    int retval = TREEMAP_SUCCESS;

    if (treemap == NULL) {
        return TREEMAP_ERROR;
    }

    // Acquire a writer lock.
    if (treemap->rwlock != NULL && (0 != lpx_rwlock_acquire_writer_lock(treemap->rwlock))) {
        return TREEMAP_ERROR;
    }

    // Now insert the value into the tree.
    retval = insert(treemap, key, value);

    // Unlock the rwlock and exit.
    if (treemap->rwlock != NULL && (0 != lpx_rwlock_release_writer_lock(treemap->rwlock))) {
        // We're hosed.
        retval = TREEMAP_ERROR;
    }

    return retval;
}

/**
 * @brief  Search the treemap for a given key.
 * @param  treemap The treemap to operate on.
 * @param  key The key to be looked up.
 * @param  value Pointer to where the value should be stored.
 * @return 0 on success, -1 on failure.
 */
int lpx_treemap_get(lpx_treemap_t *treemap, unsigned long key, unsigned long *value)
{
    int retval = TREEMAP_ERROR;

    if (treemap == NULL || value == NULL) {
        return TREEMAP_ERROR;
    }

    // Acquire a reader lock.
    if (treemap->rwlock != NULL && (0 != lpx_rwlock_acquire_reader_lock(treemap->rwlock))) {
        return TREEMAP_ERROR;
    }

    rbnode *currentNode = treemap->head;

    while (currentNode != NULL) {
        if (currentNode->key == key) {
#if PRINT_PATH
            printf("F\n");
#endif
	    *value = currentNode->value;
	    retval = TREEMAP_SUCCESS;
	    break;
	}

	if (key < currentNode->key) {
#if PRINT_PATH
            printf("L ");
#endif
	    currentNode = currentNode->left;
	} else {
#if PRINT_PATH
            printf("R ");
#endif
	    currentNode = currentNode->right;
	}
    }

    // Release reader lock.
    if (treemap->rwlock != NULL && (0 != lpx_rwlock_release_reader_lock(treemap->rwlock))) {
        // We're hosed.
        return TREEMAP_ERROR;
    }

    return retval;
}

/**
 * @brief  Remove a key value pair from the treemap.
 * @param  treemap The treemap to operate on.
 * @param  key The key to delete.
 * @return 0 on success, -1 on failure.
 */
int lpx_treemap_delete(lpx_treemap_t *treemap, unsigned long key)
{
    int retval = TREEMAP_SUCCESS;

    if (treemap == NULL) {
        return TREEMAP_ERROR;
    }

    // Acquire a writer lock.
    if (treemap->rwlock != NULL && (0 != lpx_rwlock_acquire_writer_lock(treemap->rwlock))) {
        return TREEMAP_ERROR;
    }

    // Find the node that we need to delete.
    rbnode *currentNode = treemap->head;
    while (currentNode != NULL) {
        if (currentNode->key == key) {
	    break;
	}

	if (key < currentNode->key) {
	    currentNode = currentNode->left;
	} else {
	    currentNode = currentNode->right;
	}
    }
    
    // Delete it if we found it.
    if (currentNode == NULL) {
        retval = TREEMAP_ERROR;
    } else {
        retval = delete(treemap, currentNode);
    }

    // Unlock the rwlock and exit.
    if (treemap->rwlock != NULL && (0 != lpx_rwlock_release_writer_lock(treemap->rwlock))) {
        // We're hosed.
        retval = TREEMAP_ERROR;
    }

    return retval;
}


/**
 * Deletes a node from the treemap.
 * @param treemap The treemap to delete from.
 * @param node The node to delete.
 * @return 0 on success, -1 on failure.
 */
static int delete(lpx_treemap_t *treemap, rbnode *node)
{
    return TREEMAP_SUCCESS;
}

/**
 * @brief  Destroy the treemap and release all resources. Threads calling get
 *         or put while destroy is being called will cause indeterminate behavior.
 * @param  treemap The treemap to destroy.
 * @return 0 on success, -1 on failure.
 */
int lpx_treemap_destroy(lpx_treemap_t *treemap)
{
    
    // Acquire a writer lock.
    if (treemap->rwlock != NULL && (0 != lpx_rwlock_acquire_writer_lock(treemap->rwlock))) {
        return TREEMAP_ERROR;
    }

    // Perform a post order traversal of the tree and free up resources
    // as you go.
    rbnode *currentNode = treemap->head;
    rbnode *parentNode = NULL;
    while (currentNode != NULL) {
        if (currentNode->left != NULL) {
	    // A left subtree exists so we need to destroy it.
	    currentNode = currentNode->left;
	    continue;
	}

	if (currentNode->right != NULL) {
	    // A right subtree exists so we need to destroy it.
	    currentNode = currentNode->right;
	    continue;
	}

	// Ok, so there are no left and right subtrees. Deallocate 
	// yourself and make your parent the current node.
	parentNode = currentNode->parent;
	if (parentNode == NULL) {
	    treemap->head = NULL;
	} else if (isLeftChild(parentNode, currentNode)) {
	    parentNode->left = NULL;
	} else {
	    parentNode->right = NULL;
	}

	FREE(treemap->pool, currentNode);
	currentNode = parentNode;
    }

    // Unlock the rwlock.
    if (treemap->rwlock != NULL) {
        lpx_rwlock_release_writer_lock(treemap->rwlock);
    }

    // Finally get rid of the rwlock.
    if (treemap->rwlock != NULL) {
        lpx_rwlock_destroy(treemap->rwlock);
        FREE(treemap->pool, treemap->rwlock);
    }

    return TREEMAP_SUCCESS;
}

