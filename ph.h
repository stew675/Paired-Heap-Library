// Stew's paired heap implementation

// Creates a paired-heap anchor node, around which a set of user data may be grouped under
// Returns an opaque handle to the heap, which the user may pass for other operations
//
// cmp() is a user-supplied function of the form int cmp(void *key1, void *key2), being the
// supplied keys of two nodes.  cmp() should then typecast and compare the two keys
// by whatever mechanism the user decides.  If key1 < key2, cmp() should return -1.
// If key1 > key2, then cmp() should return 1.  If both keys are equal, cmp() must return 0
//
// If a NULL value is given for cmp(), the implementation defaults to typecasting the void *key
// values to uint64_t values, and compares the results
void *pheap_create(int (*cmp)(void *, void *));

// Releases an entire paired heap tree from memory, and the anchor node as well
// oph must not be used afterwards (and its contents are zeroed out)
// void kd_free(void *key, void *data) is a caller provided function that will be
// called with every node touched to release any allocated key/data space that is
// associated with the node.  If kd_free == NULL, it is assumed that there is no
// user-allocated key/data storage and the node will simply be destroyed
void pheap_destroy(void *oph, void (*kd_free)(void *, void *));

// Inserts the user supplied key/data tuple into the paired heap.  Returns
// an opaque pointer to the heap node that is associated with the user
// data, that the user may pass to pheap_delete() later as required
void *pheap_insert(void *oph, void *key, void *data);

// Gets the key pointer associated with the given node
void *pheap_get_key(void *opn);

// Gets the data pointer associated with the given node
void *pheap_get_data(void *opn);

// Returns opaque handle to the least node in the given heap
// Sets key and data to that in the node if they are non-NULL
void *pheap_get_min_node(void *oph, void **key, void **data);

// Deletes a least node from the given paired heap. Sets key and data to
// point at the key and data that was associated with the deleted node
// Returns 0 if there was no available node to delete, and 1 if a node was
// deleted.  This is needed because if keys are used as integers, then 0
// is still a valid key value
int pheap_delete_min(void *oph, void **key, void **data);

// Deletes a node from the given paired heap in-place.
// Sets key and data to that in the node if they are non-NULL
int pheap_delete(void *oph, void *opd, void **key, void **data);

// Changes the key of the given node that is a member of the given heap
void pheap_change_key(void *oph, void *opd, void *newkey);

// Sets the data pointer associated with the given node to the new supplied value
void pheap_set_data(void *opd, void *newdata);
