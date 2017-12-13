// Stew's paired heap implementation
#include	<stdlib.h>
#include	<stdint.h>
#include	<stdio.h>

// Uncomment (or define at compile time) to turn on use of recursive pair merging
// instead of the faster iterative pair merging
//#define __PH_USE_RECURSIVE_MERGE

struct heap {
	struct heap	*next;		// Next sibling
	struct heap	*prev;		// Previous sibling or parent
	struct heap	*sub;		// child-nodes
	void		*key;		// The key to compare on
	void		*data;		// The associated data with this entry
};

struct pheap {
	int 		(*cmp)();	// User supplied compare function
	struct heap	*root;		// The root of the actual heap
};


// Default compare function that treats the void * data pointers as 64
// bit integers. This more or less allows for quick integer sorting routines
static int heap_int_cmp(void *a, void *b)
{
	int64_t n = (int64_t)a;
	int64_t p = (int64_t)b;

	if(n < p)
		return -1;
	if(n > p)
		return 1;
	return 0;
} // heap_int_cmp


// Joins two root nodes together, assuming that node a has priority
// A root-type node is a node that has no siblings, but may have children
static struct heap *
heap_join(struct heap *a, struct heap *b)
{
	if((b->next = a->sub))
		b->next->prev = b;
	b->prev = a;
	a->sub = b;
	a->next = NULL;
	a->prev = NULL;
	return a;
} // heap_join


// Merges two root-type nodes together in an heap ordered manner
static struct heap *
heap_merge(int (*cmp)(), struct heap *a, struct heap *b)
{
	int result;

	if(a == NULL) {
		b->prev = b->next = NULL;
		return b;
	}
	if(b == NULL) {
		a->prev = a->next = NULL;
		return a;
	}
	if(cmp(a->key, b->key) < 0)
		return heap_join(a, b);
	return heap_join(b, a);
} // heap_merge


// Beutifully elegant, but is likely to run us out of stack space for >1M 
// items.  For large numbers of items, or if memory sensitive, use
// the more strictly contained heap_merge_pairs_iterative() below
static struct heap *
heap_merge_pairs_recursive(int (*cmp)(), struct heap *r)
{
	struct heap *np;

	r->prev = NULL;
	if(r->next == NULL)
		return r;
	// Need record r->next->next now, as it will change after a heap_merge()
	np = r->next->next;
	return heap_merge(cmp, heap_merge(cmp, r, r->next), (np ? heap_merge_pairs_recursive(cmp, np) : NULL));
} // heap_merge_pairs_recursive


// Not as functionally elegant as heap_merge_pairs_recursive, but this won't run
// us out of stack memory if we're being used to sort many millions of elements
#define	MSN	250		// Maximum nodes we'll allocate on the stack

static struct heap *
heap_merge_pairs_iterative(int (*cmp)(), register struct heap *r)
{
	struct heap	*sn[MSN];
	register struct heap	*n, *p, **m = sn, **l = sn + MSN;

	// Isolate the sub-chain from the parent
	for(r->prev = NULL, p = r->next; p; p = r->next) {
		// Do initial left-to-right pairing pass
		for(n = p->next; r && (m < l); (r = n) && (p = r->next) ? (n = p->next) : (n = p))
			*m++ = heap_merge(cmp, r, p);

		// Now do reduction pairing pass right to left
		for(p = *--m; m > sn;)
			p = heap_merge(cmp, *--m, p);

		// Append any overflow and continue
		p->next = r;
		r = p;
	}
	return r;
} // heap_merge_pairs_iterative


// Wrapper that selects which type of recursive/iterative merging to used based upon compile time options
static struct heap *
heap_merge_pairs(int (*cmp)(), struct heap *f)
{
	if(f == NULL)
		return NULL;
#ifdef __PH_USE_RECURSIVE_MERGE
	return heap_merge_pairs_recursive(cmp, f);
#else
	return heap_merge_pairs_iterative(cmp, f);
#endif
} // heap_merge_pairs


// Unhook and free the root-type that was passed to us
// Return a new root-type node determined from any children of the node passed to us
static struct heap *
heap_del_min(int (*cmp)(), struct heap *a)
{
	struct heap *nr;

	if(a == NULL)
		return NULL;
	else
		nr = a->sub;

	free(a);

	if(nr == NULL)
		return NULL;

	return heap_merge_pairs(cmp, nr);
} // heap_del_min


// Deletes a node from the given paired heap in-place
// Returns the user-supplied data point that was associated with the deleted node
static void *
heap_del(int (*cmp)(), struct pheap *ph, struct heap *d)
{
	struct heap *s;
	void *data;

	if(d == NULL)
		return NULL;

	data = d->data;

	if(ph->root == NULL)	// Tried to delete from an empty tree
		return NULL;

	if(d == ph->root) { 	// We are the root node
		ph->root = heap_del_min(ph->cmp, ph->root);
		return data;
	}

	// d->prev can never be NULL, since we are not the root node (test case above)
	// We can therefore eliminate some checking below for speed as a result
	if(d->sub) {
		s = heap_merge_pairs(cmp, d->sub);
		s->prev = d->prev;
		if((s->next = d->next))
			s->next->prev = s;
	} else {
		if((s = d->next))
			s->prev = d->prev;
	}

	if(d->prev->sub == d)
		d->prev->sub = s;
	else
		d->prev->next = s;
	free(d);

	return data;
}


// Inserts the user supplied data pointer into the paired heap
// Returns an opaque pointer to the heap node that is associated with
// the user data, that the call may use to pass to pheap_delete()

void *
pheap_insert(void *oph, void *key, void *data)
{
	struct pheap *ph = (struct pheap *)oph;
	struct heap *n;

	// First create the new node
	n = (struct heap *)calloc(sizeof(struct heap), 1);
	if(n == NULL)
		return NULL;
	n->key = key;
	n->data = data;

	if(ph->root == NULL) {
		ph->root = n;
		return (void *)n;
	}

	ph->root = heap_merge(ph->cmp, n, ph->root);

	return n;
} // pheap_insert


// Releases an entire paired heap tree from memory, and the anchor node as well
// XXX - Somewhat slow (but memory safe) because it is sorting while deleting
//	Replace with a traversal-based free, but there are concerns about
//	potentially running out of stack space for large data set sizes (>1M)
void
pheap_destroy(void *oph)
{
	struct pheap *ph = (struct pheap *)oph;

	if(ph) {
		while((ph->root = heap_del_min(ph->cmp, ph->root)));
		free(ph);
	}
} // pheap_destroy


// Returns the user data associated with the least node in the given heap
void *
pheap_get_min(void *oph)
{
	struct pheap *ph = (struct pheap *)oph;

	if(ph->root == NULL)
		return NULL;
	return ph->root->data;
} // pheap_get_min


// Deletes a least node from the given paired heap
// Returns the user-supplied data point that was associated with the deleted node
void *
pheap_del_min(void *oph)
{
	struct pheap *ph = (struct pheap *)oph;
	void *data = NULL;

	if(ph->root) {
		data = ph->root->data;
		ph->root = heap_del_min(ph->cmp, ph->root);
	}
	return data;
} // pheap_del


// Deletes a node from the given paired heap in-place
// Returns the user-supplied data point that was associated with the deleted node
void *
pheap_del(void *oph, void *opd)
{
	struct pheap *ph = (struct pheap *)oph;
	struct heap *pd = (struct heap *)opd;

	return heap_del(ph->cmp, ph, pd);
} // pheap_del


// Changes the key of the given node that is a member of the given heap.
// Returns the opaque reference to the new node as a result of the key change
void *
pheap_decrease_key(void *oph, void *opd, void *newkey)
{
	struct pheap *ph = (struct pheap *)oph;
	struct heap *pd = (struct heap *)opd;
	struct heap *s;
	void *data;
	int res;

	if((oph == NULL) || (opd == NULL))
		return NULL;

	res = ph->cmp(newkey, pd->key);

	// Handle increase key scenario
	if(res > 0) {
		data = pheap_del(oph, opd);
		return pheap_insert(oph, newkey, data);
	}

	// Handle equivalent key scenario
	if(res == 0) {
		pd->key = newkey;		// Just set the key to the new and return
		return opd;
	}

	// From here on, we are decreasing the key of some node

	// Handle root node scenario
	if(pd == ph->root) {
		pd->key = newkey;
		return opd;
	}

	// Isolate the pd node entirely

	if(pd->sub) {
		s = heap_merge_pairs(ph->cmp, pd->sub);
		s->prev = pd->prev;
		if((s->next = pd->next))
			s->next->prev = s;
	} else {
		if((s = pd->next))
			s->prev = pd->prev;
	}

	if(pd->prev->sub == pd)
		pd->prev->sub = s;
	else
		pd->prev->next = s;
	pd->next = pd->prev = pd->sub = NULL;

	// pd is now isolated, update its key and merge it with the root node
	pd->key = newkey;
	ph->root = heap_merge(ph->cmp, ph->root, pd);
	return pd;
} // pheap_decrease_key


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
pheap_create(int (*cmp)())
{
	struct pheap *ph;

	ph = (struct pheap *)calloc(sizeof(struct pheap), 1);
	if(ph == NULL)
		return (void *)NULL;
	if(cmp == NULL)
		ph->cmp = heap_int_cmp;
	else
		ph->cmp = cmp;
	return (void *)ph;
} // pheap_create
