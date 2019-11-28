// Paired Heap Test Framework

#include        <stdio.h>
#include        <stdlib.h>
#include        <stdint.h>
#include        <time.h>
#include	"ph.h"

#define TIME_START 0
#define TIME_SETUP 1
#define TIME_DONE  2

void
test_time(int t)
{
	static struct timespec at_start, after_setup, at_done;
	double taken;

	switch(t) {
	case TIME_START:
		clock_gettime(CLOCK_REALTIME, &at_start);
		break;
	case TIME_SETUP:
		clock_gettime(CLOCK_REALTIME, &after_setup);
		taken = after_setup.tv_nsec - at_start.tv_nsec;
		taken /= 1000000000;
		taken += after_setup.tv_sec - at_start.tv_sec;
		fprintf(stderr, "Time to setup: %.3f\n", taken);
		break;
	case TIME_DONE:
		clock_gettime(CLOCK_REALTIME, &at_done);
		taken = at_done.tv_nsec - after_setup.tv_nsec;
		taken /= 1000000000;
		taken += at_done.tv_sec - after_setup.tv_sec;
		fprintf(stderr, "Time to run: %.3f\n", taken);
		break;
	default:
		fprintf(stderr, "Illegal argument to %s\n", __FUNCTION__);
		break;
	}
} // test_time


void
test(intptr_t count)
{
	void *heap = NULL, **nodes = NULL;
	intptr_t i, cnt = 0;

	if ((nodes = (void **)calloc(count, sizeof(void *))) == NULL) {
		fprintf(stderr, "Test FAILED - Out of memory\n");
		goto test_cleanup;
	}

	// Warmup - Primes L1/2/3 caches with data

	fprintf(stderr, "WARMUP START\n");
	if ((heap = pheap_create(NULL)) == NULL) {
		fprintf(stderr, "Test 1 FAILED - Failed to create heap\n");
		test_time(TIME_SETUP);
		test_time(TIME_DONE);
		goto test_cleanup;
	}
	for(i = 0; i < count; i++) {
		intptr_t key = random() % INTPTR_MAX;
		nodes[i] = pheap_insert(heap, (void *)key, (void *)i);
	}
	for(i = 0; i < count; i++) {
		pheap_delete(heap, nodes[i], NULL, NULL);
		nodes[i] = NULL;
	}
	fprintf(stderr, "WARMUP DONE\n");
	pheap_destroy(heap, NULL);
	heap = NULL;

	fprintf(stderr, "\n");

	// Baseline - Just inserts and removes with no heap merging.  Should be O(1)
	fprintf(stderr, "BASELINE - DELETE OUT OF ORDER ON INACTIVE HEAP\n");
	test_time(TIME_START);
	if ((heap = pheap_create(NULL)) == NULL) {
		fprintf(stderr, "Test 1 FAILED - Failed to create heap\n");
		test_time(TIME_SETUP);
		test_time(TIME_DONE);
		goto test_cleanup;
	}
	for(i = 0; i < count; i++) {
		intptr_t key = random() % INTPTR_MAX;
		nodes[i] = pheap_insert(heap, (void *)i, (void *)i);
	}
	fprintf(stderr, "BASELINE SETUP DONE. Inserted %ld nodes into empty heap\n", count);
	test_time(TIME_SETUP);
	for(i = count - 1; i >= 0; i--) {
		pheap_delete(heap, nodes[i], NULL, NULL);
		nodes[i] = NULL;
	}
	fprintf(stderr, "BASELINE DONE. Deleted %ld nodes from inactive heap\n", count);
	test_time(TIME_DONE);
	pheap_destroy(heap, NULL);
	heap = NULL;

	fprintf(stderr, "\n");

	// Test 1 - Insert count nodes, then (delete min + insert)count/8 times to force heap
	// to do merging, and then just delete the whole lot out of order according to our list
	fprintf(stderr, "TEST 1 - DELETE OUT OF ORDER ON ACTIVE HEAP\n");
	test_time(TIME_START);
	if ((heap = pheap_create(NULL)) == NULL) {
		fprintf(stderr, "Test 1 FAILED - Failed to create heap\n");
		test_time(TIME_SETUP);
		test_time(TIME_DONE);
		goto test_cleanup;
	}
	for(i = 0; i < count; i++) {
		intptr_t key = random() % INTPTR_MAX;
		nodes[i] = pheap_insert(heap, (void *)key, (void *)i);
	}
	fprintf(stderr, "Test 1 SETUP Phase 1 - Inserted %ld nodes into empty heap.\n", count);
	test_time(TIME_SETUP);

	// Now add and delete 1/8 of count of nodes to force heap to merge a lot
	test_time(TIME_START);
	for(i = 0; i < (count >> 3); i++) {
		intptr_t pos;
		pheap_delete_min(heap, NULL, (void **)&pos);
		intptr_t key = random() % INTPTR_MAX;
		nodes[pos] = pheap_insert(heap, (void *)key, (void *)pos);
	}
	fprintf(stderr, "Test 1 SETUP Phase 2 - Activated heap with Delete Min + Insert %ld times\n", (count >> 3));
	test_time(TIME_SETUP);

	// Now delete all the nodes from our copy of the node list
	for (i = 0; i < count; i++) {
		pheap_delete(heap, nodes[i], NULL, NULL);
		nodes[i] = NULL;
	}
	fprintf(stderr, "Test 1 DONE - Deleted %ld nodes out of order\n", i);
	test_time(TIME_DONE);
	pheap_destroy(heap, NULL);
	heap = NULL;

	fprintf(stderr, "\n");

	// Test 2 - Insert count nodes, then delete min the lot, forcing a full in-order removal
	fprintf(stderr, "TEST 2 - DELETE/SORT IN ORDER\n");
	test_time(TIME_START);
	if ((heap = pheap_create(NULL)) == NULL) {
		fprintf(stderr, "Test 2 FAILED - Failed to create heap\n");
		test_time(TIME_SETUP);
		test_time(TIME_DONE);
		goto test_cleanup;
	}
	for(i = 0; i < count; i++) {
		intptr_t key = random() % INTPTR_MAX;
		nodes[i] = pheap_insert(heap, (void *)key, (void *)i);
	}
	fprintf(stderr, "Test 2 SETUP - Inserted %ld nodes into empty heap\n", count);
	test_time(TIME_SETUP);

	// Now delete in order
	while(pheap_delete_min(heap, NULL, (void **)&i)) {
		nodes[i] = NULL;
		cnt++;
	}
	fprintf(stderr, "Test 2 DONE - Sorted and Deleted %ld nodes in order\n", cnt);
	test_time(TIME_DONE);
	pheap_destroy(heap, NULL);
	heap = NULL;

	// Cleanup
test_cleanup:
	if (heap) {
		pheap_destroy(heap, NULL);
		heap = NULL;
	}
	if (nodes) {
		free(nodes);
		nodes = NULL;
	}
} // test


int
main(int argc, char *argv[])
{
	intptr_t count;
	if(argc != 2) {
		fprintf(stderr, "Usage: %s count\n", argv[0]);
		return 0;
	}
	count = (intptr_t)atoi(argv[1]);
	if(count < 1) {
		fprintf(stderr, "%s: count must be an integer of 1 or greater\n", argv[0]);
		return 0;
	}

	test(count);
	fprintf(stderr, "\n");
} // main
