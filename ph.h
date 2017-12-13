// Stew's paired heap implementation

// Creates a paired-heap anchor node, around which a set of user data may be grouped under
// Returns an opaque handle to the heap, which the user may pass for other operations
void *pheap_create(int (*cmp)());

// Releases an entire paired heap tree from memory, and the anchor node as well
void pheap_destroy(void *oph);

// Inserts the user supplied data pointer into the paired heap
// Returns an opaque pointer to the heap node that is associated with
// the user data, that the call may use to pass to pheap_delete()
void *pheap_insert(void *oph, void *key, void *data);

// Returns the user data associated with the least node in the given heap
void *pheap_get_min(void *oph);

// Deletes a least node from the given paired heap
// Returns the user-supplied data point that was associated with the deleted node
void *pheap_del_min(void *oph);

// Deletes a node from the given paired heap in-place
// Returns the user-supplied data point that was associated with the deleted node
void *pheap_del(void *oph, void *opd);

// Changes the key of the given node that is a member of the given heap.
// Returns the opaque reference to the new node as a result of the key change
void *pheap_decrease_key(void *oph, void *opd, void *newkey);

// Sets the data pointer associated with some node to the new supplied value
void pheap_set_data(void *opd, void *newdata);
