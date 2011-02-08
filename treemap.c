/**
 * @file   treemap.c
 * @author Rakesh Iyer
 * @brief  An implementation of a map based on a red black tree. For an
 *         explanation of how this worked go to:
 *         [1] http://en.wikipedia.org/wiki/Red-black_tree
 *         [2] http://www.ece.uc.edu/~franco/C321/html/RedBlack/redblack.html
 *
 *         The implementation is non recursive... just because...
 *         The assertion routine is recursive though. Because I was lazy :)
 *
 * @bug    Still needs a lot of testing. Mostly works though.
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
#include <stdio.h>

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
static inline rbnode *sibling(rbnode *node);
static inline int isLeftChild(rbnode *parent, rbnode *child);
static inline int isRightChild(rbnode *parent, rbnode *child);
static void rotateLeft(lpx_treemap_t *treemap, rbnode *node);
static void rotateRight(lpx_treemap_t *treemap, rbnode *node);
static int delete(lpx_treemap_t *treemap, rbnode *node);
static rbnode *findReplacementCandidate(lpx_treemap_t *treemap, rbnode *node);
static rbnode *deleteReplacementCandidate(lpx_treemap_t *treemap, rbnode *node, rbnode *candidate);
static inline int hasRedChild(rbnode *node); 
static int assert_rb_valid(lpx_treemap_t *treemap, rbnode *node, int blackCount, int *maxBlackCount);
static rbnode *predecessor(lpx_treemap_t *treemap, rbnode *node);
static rbnode *successor(lpx_treemap_t *treemap, rbnode *node);

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
    if (node->parent != NULL) {
        unode = sibling(node->parent);
    }
    return unode;
}

/**
 * @brief  Get the other child of the given nodes parent.
 * @param  node The node whose sibling is required.
 * @return The sibling if it exists, NULL if not.
 */
static inline rbnode *sibling(rbnode *node)
{
    rbnode *sibling = NULL;
    // Root nodes have no sibling.
    if (node->parent != NULL) {
	if (isLeftChild(node->parent, node)) {
	    sibling = node->parent->right;
	} else {
	    sibling = node->parent->left;
	}
    }
    return sibling;
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
	    *value = currentNode->value;
	    retval = TREEMAP_SUCCESS;
	    break;
	}

	if (key < currentNode->key) {
	    currentNode = currentNode->left;
	} else {
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

    // Find the node that we need to delete. There cost of this delete is 2 log (n), one
    // so that we find the node, and the other step to actually delete the node. Not too
    // bad I guess.
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
 * @brief  Find the inorder predecessor or successor which we want to swap
 *         values with and delete. We do this while also modifying the colors
 *         of the nodes that we cross so that the replacement candidate is 
 *         always a red node.
 * @param  treemap The map to look in.
 * @param  node The node to find a replacement for.
 * @return A valid node if found, NULL if not.
 */
static rbnode *findReplacementCandidate(lpx_treemap_t *treemap, rbnode *node)
{
    rbnode *candidate = NULL;
    rbnode *pnode = NULL;
    rbnode *snode = NULL;

    // First, find the replacement candidate.
    if ((pnode = predecessor(treemap, node)) != NULL) {
        // Get the inorder predecessor.
	candidate = pnode;
    } else if ((snode = successor(treemap, node)) != NULL) {
        // Get the inorder successor.
	candidate = node->right;
    }

    return candidate;
}

/**
 * @brief  Get the inorder predecessor of the given node.
 * @param  treemap The treemap to operate on.
 * @param  node The node whose predecessor is requested.
 * @return The precedecessor if it exists, NULL if not.
 */
static rbnode *predecessor(lpx_treemap_t *treemap, rbnode *node)
{
    rbnode *predecessor = NULL;
    if (node->left != NULL) {
        predecessor = node->left;
	while(predecessor->right != NULL) {
	    predecessor = predecessor->right;
	}
    }

    return predecessor;
}

/**
 * @brief  Get the inorder predecessor of the given node.
 * @param  treemap The treemap to operate on.
 * @param  node The node whose predecessor is requested.
 * @return The precedecessor if it exists, NULL if not.
 */
static rbnode *successor(lpx_treemap_t *treemap, rbnode *node)
{
    rbnode *successor = NULL;
    return successor;
}

/**
 * @brief Check if a node has atleast one red child.
 * @param node The node to check.
 * @return 1 if true, 0 if not.
 */
static inline int hasRedChild(rbnode *node) 
{
    if (isRed(node->right) || isRed(node->left)) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * @brief  Get the far nephew of a node.
 * @param  The node to check.
 * @return The far nephew if it exists, NULL otherwise.
 */
static inline rbnode *farnephew(rbnode *node)
{
    rbnode *parent = node->parent;
    rbnode *siblingNode = sibling(node);
    rbnode *farNephew = NULL;
    
    if (siblingNode != NULL) {
        if (isRightChild(parent, siblingNode)) {
            farNephew = siblingNode->right;
        } else {
            farNephew = siblingNode->left;
        }
    }

    return farNephew;
}

/**
 * @brief  Delete the replacement node for the node that we actually want to delete.
 * @param  treemap The treemap we are operating on.
 * @param  node The actual node to be deleted.
 * @param  candidate The node that is being deleted instead.
 * @return The node that needs to be freed.
 */
static rbnode *deleteReplacementCandidate(lpx_treemap_t *treemap, rbnode *node, rbnode *candidate)
{

    // Handle the simple cases.

    if (isRed(candidate) && candidate->left == NULL && candidate->right == NULL) {
        // Red leaves can be simply deleted.
	if (isLeftChild(candidate->parent, candidate)) {
	    candidate->parent->left = NULL;
	} else {
	    candidate->parent->right = NULL;
	}
	return candidate;
    }

    if (isBlack(candidate) && hasRedChild(candidate)) {
        // The child must be red or else the tree is not a valid red black tree.
        rbnode *child = (candidate->left != NULL) ? candidate->left : candidate->right;
	candidate->key = child->key;
	candidate->value = child->value;
	candidate->left = NULL;
	candidate->right = NULL;
	return child;
    }

    // Right now the candidate is black and has no children. No other possibilities exist.
    rbnode *current = candidate;
    while(current->parent != NULL) {
        rbnode *parent = current->parent;
        rbnode *siblingNode = sibling(current);
	if (isRed(siblingNode)) {
	    // Make the sibling black, the parent red and rotate around the parent node.
	    // 5.1.1 Exchange the parent and sibling colors.
	    siblingNode->color = COLOR_BLACK;
	    parent->color = COLOR_RED;
	    // 5.1.2 Rotate the sibling into the place of the parent.
	    if (isLeftChild(parent, current)) {
	        rotateLeft(treemap, parent);
	    } else {
	        rotateRight(treemap, parent);
	    }

	    continue;
	}

	// We know for sure that candidates sibling is black.
	// Sibling is black with 2 black children.
	if (isBlack(siblingNode->left) && isBlack(siblingNode->right)) {
	    // 5.2.1 Make the sibling red.
	    siblingNode->color = COLOR_RED;
	    // Make the parent the current node. If the current is black, there is a black imbalance so continue.
	    current = parent;
	    if (parent->color == COLOR_BLACK) {
	        continue;
	    }

	    // 5.2.2 The current is red, make it black. There are no more conflicts so terminate.
	    current->color = COLOR_BLACK;
	    break;
	}
	
	// Sibling is black with one or more red children
	if (isRed(siblingNode->left) || isRed(siblingNode->right)) {
	    rbnode *farNephew = farnephew(current);
	    
	    if (isBlack(farNephew)) {
	        // 5.3.1 If the far nephew is black, rotate in that direction around the sibling.
	        if (isLeftChild(siblingNode, farNephew)) {
	            rotateLeft(treemap, siblingNode);
	        } else {
	            rotateRight(treemap, siblingNode);
	        }
	    }

	    // 5.3.2 Set the far nephew to black, the sibling to the color of the parent and the parent to black.
	    siblingNode = sibling(current);
	    farNephew = farnephew(current);
	    farNephew->color = COLOR_BLACK;
	    siblingNode->color = siblingNode->parent->color;
	    current->parent->color = COLOR_BLACK;

	    // 5.3.3 Rotate around the parent in the direction of current. If a predecessor or successor
	    //       exists, that becomes the new candidate node.
	    if (isLeftChild(current->parent, current)) {
	        rotateLeft(treemap, current->parent);
	    } else {
	        rotateRight(treemap, current->parent);
	    }

            // TODO: Why was this there? - If a predecessor or successor exits to current, make that the new candidate node.
	    break;
	}
    }

    return candidate;
}

/**
 * Deletes a node from the treemap.
 * @param treemap The treemap to delete from.
 * @param node The node to delete.
 * @return 0 on success, -1 on failure.
 */
static int delete(lpx_treemap_t *treemap, rbnode *node)
{
    long key = 0;
    long value = 0;

    rbnode *candidate = findReplacementCandidate(treemap, node);
    if (candidate == NULL) {
        // The node that we want to delete is a leaf node. We dont need
	// to copy anything in.
	candidate = node;
    }

    candidate = deleteReplacementCandidate(treemap, node, candidate);
    if (candidate != node) {
        // We have a replacement candidate to delete. Copy the value
	// of the replacement into the node that we want to delete.
	key = candidate->key;
	value = candidate->value;
    }

    // Finally free the candidate. If we had deleted a replacement candidate, we copy the
    // data of the replacement into the node that should have been deleted.
    rbnode *parent = candidate->parent;
    FREE(treemap->pool, candidate);
    if (node != candidate) {
        node->key = key;
	node->value = value;
    }

    // Set the head to NULL if we just blew it away, otherwise set it to black.
    if (treemap->head == candidate) {
        treemap->head = NULL;
    } else {
        // Unlink the candidate node that we want to delete.
        if (isLeftChild(parent, candidate)) {
	    parent->left = NULL;
	} else {
	    parent->right = NULL;
	}

        treemap->head->color = COLOR_BLACK;
    }

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

/**
 * @brief  Check if the given subtree is a valid red black tree.
 * @param  treemap The treemap to check.
 * @param  node The node that represents the root of the subtree that needs to be checked.
 * @param  blackCount The black count at this level.
 * @param  maxBlackCount The maximum black depth at this node.
 * @return 0 if its a valid rb tree, -1 if not.
 */
static int assert_rb_valid(lpx_treemap_t *treemap, rbnode *node, int blackCount, int *maxBlackCount)
{
    int retval = 0;

    // A red node must have 2 black children.
    if (isRed(node)) {
        if (!isBlack(node->left) || !isBlack(node->right)) {
	    printf("!! Red node has non red child !!\n");
            return TREEMAP_ERROR;
        }
    } else {
        // Increase the black count.
	blackCount++;
    }

    if (node->left == NULL && node->right == NULL) {
	// Black depth must be equal for all leaf nodes.
	if (*maxBlackCount == -1) {
	    *maxBlackCount = blackCount;
	} else {
	    if (*maxBlackCount != blackCount) {
	        printf("!! Black count violation Expected %d, got %d !!\n", *maxBlackCount, blackCount);
	        return TREEMAP_ERROR;
	    }
	}
    }

    // Verify the left subtree.
    if (node->left != NULL) {
        retval |= assert_rb_valid(treemap, node->left, blackCount, maxBlackCount);
    }

    // Verify the right subtree.
    if (node->right != NULL) {
        retval |= assert_rb_valid(treemap, node->right, blackCount, maxBlackCount);
    }

    return retval;
}

/**
 * @brief  Check whether the treemap is a valid rb tree.
 * @param  treemap The treemap to check for conflicts.
 * @return 0 if no conflicts are found, -1 otherwise
 */
int lpx_treemap_check_rb_conflicts(lpx_treemap_t *treemap)
{
    int maxBlackCount = -1;

    if (treemap->head != NULL) {
        return assert_rb_valid(treemap, treemap->head, 0, &maxBlackCount);
    } else {
        return TREEMAP_SUCCESS;
    }
}

