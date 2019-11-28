// Stew's paired heap implementation
#include	<stdlib.h>
#include	<stdint.h>
#include	<string.h>
#include	"ph.h"

// Uncomment (or define at compile time) to turn on use of recursive pair merging
// instead of the (slightly) faster iterative pair merging
// #define __PH_USE_RECURSIVE_MERGE

// Uncomment (or define at compile time) to turn on use of recursive tree destroy
// Recommended to use the recursive destroy, unless running into stack memory issues
#define __PH_USE_RECURSIVE_DESTROY

struct heap {
	struct heap	*next;			// Next sibling
	struct heap	*prev;			// Previous sibling or parent
	struct heap	*sub;			// child-nodes
	void		*key;			// The key to compare on
	void		*data;			// The associated data with this entry
};

struct pheap {
	int 		(*cmp)(void *, void *);	// User supplied compare function
	struct heap	*root;			// The root of the actual heap
};


// Default compare function that treats the void *key pointers as intptr_t
// integers. This allows for quick integer key queue sorting implementations
// The heap sorting doesn't explicitly handle equality so it doesn't matter
// which we choose so long as we are consistent.  However pheap_change_key()
// does care about equality. There a decrease key operation on the root node
// works the same as equality though, so we'll treat equality as simply less
// than for speed.  This can create slightly more work if the user changes
// the key of a non-root node to the exact same value as it was before, but
// we'll trust the caller of pheap_change_key() not to do that too often :)
static inline int heap_int_cmp(register void *a, register void *b)
{
	return ((intptr_t)a > (intptr_t)b ? 1 : -1);
} // heap_int_cmp


// Joins two root nodes together, assuming that node 'a' has priority
// A root-type node is a node that has no siblings, but may have children
static struct heap *
heap_join(register struct heap *a, register struct heap *b)
{
	if ((b->next = a->sub))
		b->next->prev = b;
	b->prev = a;
	a->sub = b;
	a->next = NULL;
	a->prev = NULL;
	return a;
} // heap_join


// Merges two root-type nodes together in an heap ordered manner
static struct heap *
heap_merge(int (*cmp)(void *, void *), register struct heap *a, register struct heap *b)
{
	if (a == NULL) {
		b->prev = b->next = NULL;
		return b;
	}
	if (b == NULL) {
		a->prev = a->next = NULL;
		return a;
	}
	if (cmp(a->key, b->key) < 0)
		return heap_join(a, b);
	return heap_join(b, a);
} // heap_merge

#ifdef __PH_USE_RECURSIVE_MERGE

// Functionally elegant, but is likely to run us out of stack space for >1M 
// items.  For large numbers of items, or if memory sensitive, use the
// strictly memory constrained heap_merge_pairs_iterative() below
static struct heap *
heap_merge_pairs_recursive(register int (*cmp)(), register struct heap *r)
{
	register struct heap *np;

	r->prev = NULL;
	if (r->next == NULL)
		return r;
	// Need to record r->next->next now, as r->next will change after a heap_merge()
	np = r->next->next;
	return heap_merge(cmp, heap_merge(cmp, r, r->next), (np ? heap_merge_pairs_recursive(cmp, np) : NULL));
} // heap_merge_pairs_recursive

#else

// This function is my direct pride and joy of this library. While it is not as
// functionally elegant as heap_merge_pairs_recursive, but this implementation
// is wholly memory constrained by iterating repeatedly over a stack allocated
// list of node pointers.  This won't run us out of stack memory when sorting
// many millions of elements.  The chunked-style merge-passing also appears to
// better balance the heap for large set sizes, and it outperforms the original
// merge pairing algorithm (see heap_merge_pairs_recursive() above) by around
// 10% in practise
#define	MSN	240	// Number of node pointers we'll allocate on the stack
static struct heap *
heap_merge_pairs_iterative(register int (*cmp)(void *, void *), register struct heap *r)
{
	struct heap	*sn[MSN];
	register struct heap	*n, *p, **m = sn, **l = sn + MSN;

	// Isolate the sub-chain from the parent.  Append any remainder with each pass
	for(r->prev = NULL, p = r->next; p; p->next = r, r = p, p = p->next) {
		// Do initial left-to-right pairing pass, then a reduction pairing pass right to left
		for(n = p->next; r && (m < l); *m++ = heap_merge(cmp, r, p), (r = n) && (p = r->next) ? (n = p->next) : (n = p));
		for(p = *--m; m > sn; p = heap_merge(cmp, *--m, p));
	}
	return r;
} // heap_merge_pairs_iterative

#endif

// Wrapper that selects which type of merging (recursive or iterative) to use based
// upon compile time options
static struct heap *
heap_merge_pairs(int (*cmp)(void *, void *), struct heap *r)
{
	if (r == NULL)
		return NULL;
#ifdef __PH_USE_RECURSIVE_MERGE
	return heap_merge_pairs_recursive(cmp, r);
#else
	return heap_merge_pairs_iterative(cmp, r);
#endif
} // heap_merge_pairs


// Unhook and free the root-type node that was passed to us. Return a new
// root-type node determined from any children of the node passed to us
static struct heap *
heap_delete_min(int (*cmp)(void *, void *), struct heap *d, void (*kd_free)(void *, void *))
{
	struct heap *nr;

	if (d == NULL)
		return NULL;
	else
		nr = d->sub;

	if (kd_free) {
		kd_free(d->key, d->data);
	}
	memset(d, 0, sizeof(struct heap));
	free(d);

	if (nr == NULL)
		return NULL;

	return heap_merge_pairs(cmp, nr);
} // heap_delete_min


// Detaches the node from the heap.  d MUST NOT be the root node
static void
heap_detach(int (*cmp)(void *, void *), register struct heap *d)
{
	register struct heap *s;

	// d->prev can never be NULL, since we are not the root node
	// We can eliminate some checking for speed as a result
	if (d->sub) {
		s = heap_merge_pairs(cmp, d->sub);
		s->prev = d->prev;
		if ((s->next = d->next))
			s->next->prev = s;
	} else {
		if ((s = d->next))
			s->prev = d->prev;
	}

	if (d->prev->sub == d)
		d->prev->sub = s;
	else
		d->prev->next = s;
} // heap_detach


// Deletes a node from the given paired heap in-place.
static void
heap_delete(int (*cmp)(void *, void *), struct pheap *ph, struct heap *d, void (*kd_free)(void *, void *))
{
	if (d == ph->root) { 	// We are the root node
		ph->root = heap_delete_min(ph->cmp, ph->root, kd_free);
		return;
	}

	heap_detach(cmp, d);

	if (kd_free) {
		kd_free(d->key, d->data);
	}
	memset(d, 0, sizeof(struct heap));
	free(d);
} // heap_delete


// Inserts the user supplied key/data tuple into the paired heap.  Returns
// an opaque pointer to the heap node that is associated with the user
// data, that the user may pass to pheap_delete() later as required
void *
pheap_insert(void *oph, void *key, void *data)
{
	struct pheap *ph = (struct pheap *)oph;
	struct heap *n;

	// First create the new node
	n = (struct heap *)calloc(sizeof(struct heap), 1);
	if (n == NULL)
		return NULL;
	n->key = key;
	n->data = data;

	if (ph->root == NULL) {
		ph->root = n;
		return (void *)n;
	}

	ph->root = heap_merge(ph->cmp, n, ph->root);

	return n;
} // pheap_insert

#ifdef __PH_USE_RECURSIVE_DESTROY

// Much faster breadth-iterative recursive-sub tree destroy operation
// May be stack memory sensitive for extremely deep trees, but these
// are very unlikely to develop in regular operation
// Recommend to use this, and only switch to the slower (but stack
// memory conservative heap_delete_min()) if required
static void
pheap_destroy_recursive(struct heap *n, void (*kd_free)(void *, void *))
{
	struct heap *ns = NULL;	// Next scan pointer

	while (n) {
		ns = n->next;
		pheap_destroy_recursive(n->sub, kd_free);
		if (kd_free) {
			kd_free(n->key, n->data);
		}
		memset(n, 0, sizeof(struct heap));
		free(n);
		n = ns;
	}
} // pheap_destroy_recursive

#endif

// Releases an entire paired heap tree from memory, and the anchor node as well
// oph must not be used afterwards (and its contents are zeroed out)
// kd_free is a user supplied void function to which the pheap library will pass
// the node's key and data values to before destruction, so that the caller may
// perform any necessary memory cleanup of the keys and data as required
// void kd_free(void *key, void *data);
void
pheap_destroy(void *oph, void (*kd_free)(void *, void *))
{
	struct pheap *ph = (struct pheap *)oph;

	if (ph) {
#ifdef __PH_USE_RECURSIVE_DESTROY
		pheap_destroy_recursive(ph->root, kd_free);
#else
		while((ph->root = heap_delete_min(ph->cmp, ph->root, kd_free)));
#endif
		memset(ph, 0, sizeof(struct pheap));
		free(ph);
	}
} // pheap_destroy


void *
pheap_get_key(void *ohn)
{
	struct heap *hn = (struct heap *)ohn;

	return (hn == NULL) ? NULL : hn->key;
} // pheap_get_key


void *
pheap_get_data(void *ohn)
{
	struct heap *hn = (struct heap *)ohn;

	return (hn == NULL) ? NULL : hn->data;
} // pheap_get_data


// Returns handle to the least node in the given heap
// Sets key and data if they are non-NULL
void *
pheap_get_min_node(void *oph, void **key, void **data)
{
	struct pheap *ph = (struct pheap *)oph;

	if (ph) {
		if (ph->root) {
			if (key)
				*key = ph->root->key;
			if (data)
				*data = ph->root->data;
		}
		return ph->root;
	}
	if (key)
		*key = NULL;
	if (data)
		*data = NULL;
	return NULL;
} // pheap_get_min_node


// Deletes a least node from the given paired heap. Sets key and data to
// point at the key and data that was associated with the deleted node
// Returns 1 if a node was found
// Returns 0 if the heap was already empty and there was nothing to delete
int
pheap_delete_min(void *oph, void **key, void **data)
{
	if (pheap_get_min_node(oph, key, data)) {
		struct pheap *ph = (struct pheap *)oph;

		ph->root = heap_delete_min(ph->cmp, ph->root, NULL);
		return 1;
	}
	return 0;
} // pheap_delete_min


// Deletes a node from the given paired heap in-place.  Sets key and data to
// point at the key and data that was associated with the deleted node
// Returns 1 if the deletion was successful
// Returns 0 if the deletion failed due to invalid parameters
int
pheap_delete(void *oph, void *opd, void **key, void **data)
{
	register struct pheap *ph = (struct pheap *)oph;
	register struct heap *pd = (struct heap *)opd;

	// Don't try to delete a NULL node
	if (pd == NULL)
		return 0;
	if (key)
		*key = pd->key;
	if (data)
		*data = pd->data;
	// Don't try to delete from an empty or non-existent heap
	if ((ph == NULL) || (ph->root == NULL))
		return 0;

	heap_delete(ph->cmp, ph, pd, NULL);
	return 1;
} // pheap_delete


// Changes the key of the given node that is a member of the given heap.
void
pheap_change_key(void *oph, void *opd, void *newkey)
{
	register struct pheap *ph = (struct pheap *)oph;
	register struct heap *pd = (struct heap *)opd;
	register int res;

	// Don't try to modify an empty or non-existent heap
	// Don't try to modify a NULL node
	if ((oph == NULL) || (opd == NULL) || (ph->root == NULL))
		return;

	res = ph->cmp(newkey, pd->key);		// Record the key change type

	// Set key to newkey, in case the user modifies the memory
	// presently associated with pd->key after return
	// Safe to set it now, since we recorded the change type
	pd->key = newkey;

	// Handle mega-easy key equivalence scenario
	if (res == 0)
		return;

	if (pd == ph->root) { 				// The node == root-node scenarios
		if (res < 0)				// Handle decrease key on root node
			return;

		if (pd->sub == NULL)			// Increase key with no children
			return;

		// Detach the root node and update the root pointer with new root
		ph->root = heap_merge_pairs(ph->cmp, pd->sub);
	} else {
		// Increase or decrease key, doesn't matter, it's the same operation
		heap_detach(ph->cmp, pd);		// detach the node from the heap
	}

	pd->next = pd->prev = pd->sub = NULL;		// pd references nothing else now

	// Merge pd with the root node
	ph->root = heap_merge(ph->cmp, ph->root, pd);
} // pheap_change_key


// Sets the data pointer associated with some node to the new supplied value
void
pheap_set_data(void *opd, void *newdata)
{
	struct heap *pd = (struct heap *)opd;

	pd->data = newdata;
} // pheap_set_data


// Creates a paired-heap anchor node, around which a set of user data may be grouped under
// Returns an opaque handle to the heap, which the user may pass for other operations
void *
pheap_create(int (*cmp)(void *, void *))
{
	struct pheap *ph;

	ph = (struct pheap *)calloc(sizeof(struct pheap), 1);
	if (ph == NULL)
		return NULL;
	if (cmp == NULL)
		ph->cmp = heap_int_cmp;
	else
		ph->cmp = cmp;
	return (void *)ph;
} // pheap_create
