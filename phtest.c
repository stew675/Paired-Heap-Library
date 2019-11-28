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
test1(intptr_t count)
{
	void *heap, *data;
	intptr_t i, ex, lex;

	fprintf(stderr, "TEST 1 - SORTING\n");

	// Setup phase
	test_time(TIME_START);
	if((heap = pheap_create(NULL)) == NULL) {
		fprintf(stderr, "Test 2 FAILED - Out of memory\n");
		test_time(TIME_SETUP);
		test_time(TIME_DONE);
		goto t1cleanup;
	}
	for (i = 0; i < count; i++) {
		ex = (intptr_t)random() % INTPTR_MAX;
		pheap_insert(heap, (void *)ex, (void *)ex);
	}
	test_time(TIME_SETUP);

	// Sorting phase
	i = ex = lex = 0;
	while (pheap_delete_min(heap, NULL, &data)) {
		ex = (intptr_t)data;
		if(ex < lex)
			break;
		lex = ex;
		i++;
	}
	fprintf(stderr, "Test 1 DONE - Sorted %ld nodes\n", i);
	test_time(TIME_DONE);

	// Validate
	if (ex < lex) {
		fprintf(stderr, "Test 1 FAILED - Sort out of order after %ld deletions\n", i);
	} else if (pheap_delete_min(heap, NULL, &data)) {
		fprintf(stderr, "Test 1 FAILED - Tree is not empty\n");
	} else {
		fprintf(stderr, "Test 1 PASSED - Sort is in order\n");
	}

t1cleanup:
	// Cleanup
	if (heap) {
		pheap_destroy(heap, NULL);
		heap = NULL;
	}
} // test1


void
test2(intptr_t count)
{
	void *heap, *data, **t2nodes = NULL;
	intptr_t i, ex, lex, *keys = NULL;

	fprintf(stderr, "TEST 2 - DELETE IN PLACE\n");

	// Setup phase
	test_time(TIME_START);
	if ((t2nodes = (void **)calloc(count, sizeof(void *))) == NULL) {
		fprintf(stderr, "Test 2 FAILED - Out of memory\n");
		test_time(TIME_SETUP);
		test_time(TIME_DONE);
		goto t2cleanup;
	}
	if ((keys = (intptr_t *)calloc(count, sizeof(intptr_t))) == NULL) {
		fprintf(stderr, "Test 2 FAILED - Out of memory\n");
		test_time(TIME_SETUP);
		test_time(TIME_DONE);
		goto t2cleanup;
	}
	if ((heap = pheap_create(NULL)) == NULL) {
		fprintf(stderr, "Test 2 FAILED - Out of memory\n");
		test_time(TIME_SETUP);
		test_time(TIME_DONE);
		goto t2cleanup;
	}
	for(i = 0; i < count; i++) {
		ex = (intptr_t)random() % INTPTR_MAX;
		keys[i] = ex;
		t2nodes[i] = pheap_insert(heap, (void *)ex, (void *)ex);
	}
	fprintf(stderr, "Test 2 SETUP - Inserted %ld nodes\n", i);
	test_time(TIME_SETUP);

	// Out of order deletion/validation phase
	for(i = 0; i < count; i++) {
		pheap_delete(heap, t2nodes[i], (void *)&ex, NULL);
		if(ex != keys[i])
			break;
	}
	fprintf(stderr, "Test 2 DONE - Deleted %ld nodes\n", i);
	test_time(TIME_DONE);

	// Validate
	if(i < count)
		fprintf(stderr, "Test 2 FAILED - Mismatched keys after %ld deletions\n", i);
	else if (pheap_delete_min(heap, NULL, &data))
		fprintf(stderr, "Test 2 FAILED - Tree is not empty\n");
	else
		fprintf(stderr, "Test 2 PASSED\n");

t2cleanup:
	// Cleanup
	if (t2nodes) {
		free(t2nodes);
	}
	if (keys) {
		free(keys);
	}
	if (heap) {
		pheap_destroy(heap, NULL);
		heap = NULL;
	}
} // test2


void
test3(intptr_t count)
{
	void *heap, *key, *data, **t3nodes = NULL;
	intptr_t i, ex, lex, *keys = NULL;

	fprintf(stderr, "TEST 3 - DECREASE(CHANGE) KEY and SORT\n");

	// Setup Phase
	test_time(TIME_START);
	if((t3nodes = (void **)calloc(count, sizeof(void *))) == NULL) {
		fprintf(stderr, "Test 2 FAILED - Out of memory\n");
		test_time(TIME_SETUP);
		test_time(TIME_DONE);
		goto t3cleanup;
	}
	if((keys = (intptr_t *)calloc(count, sizeof(intptr_t))) == NULL) {
		fprintf(stderr, "Test 2 FAILED - Out of memory\n");
		test_time(TIME_SETUP);
		test_time(TIME_DONE);
		goto t3cleanup;
	}
	if((heap = pheap_create(NULL)) == NULL) {
		fprintf(stderr, "Test 2 FAILED - Out of memory\n");
		test_time(TIME_SETUP);
		test_time(TIME_DONE);
		goto t3cleanup;
	}
	for(i = 0; i < count; i++) {
		ex = (intptr_t)random() % INTPTR_MAX;
		keys[i] = ex;
		t3nodes[i] = pheap_insert(heap, (void *)ex, (void *)ex);
	}
	fprintf(stderr, "Test 3 SETUP - Inserted %ld nodes\n", i);
	test_time(TIME_SETUP);

	// Now change all the keys
	for(i = 0; i < count; i++) {
		ex = random() % INTPTR_MAX;
		keys[i] = ex;
		pheap_change_key(heap, t3nodes[i], (void *)ex);
		// Since we're using the data value to echo the key value
		// to test sort/heap integrity, then update its value here
		pheap_set_data(t3nodes[i], (void *)ex);
	}
	fprintf(stderr, "Test 3 DONE - Changed Keys of %ld nodes\n", i);
	test_time(TIME_DONE);

	// Validate
	if(i < count) {
		fprintf(stderr, "Test 3 FAILED - Mismatched keys after %ld deletions\n", i);
		goto t3cleanup;
	}

	// Delete/sort validation phase
	test_time(TIME_START);
	fprintf(stderr, "Test 3 - Sorting AND Deleting\n");
	test_time(TIME_SETUP);
	ex = 0;
	lex = 0;
	i = 0;
	while(pheap_delete_min(heap, &key, &data)) {
		ex = (intptr_t)data;
		if(ex < lex)
			break;
		lex = ex;
		i++;
	}
	fprintf(stderr, "Test 3 DONE - Sorted and Deleted %ld nodes\n", i);
	test_time(TIME_DONE);

	// Validate
	if(i < count)
		fprintf(stderr, "Test 3 FAILED - Out of order after  %ld deletions\n", i);
	else if (pheap_delete_min(heap, NULL, &data))
		fprintf(stderr, "Test 3 FAILED - Tree is not empty\n");
	else
		fprintf(stderr, "Test 3 PASSED\n");

	// Cleanup
t3cleanup:
	if(t3nodes) {
		free(t3nodes);
		t3nodes = NULL;
	}
	if(keys) {
		free(keys);
		keys = NULL;
	}
	if (heap) {
		pheap_destroy(heap, NULL);
		heap = NULL;
	}
} // test3


void
test4(intptr_t count)
{
	void *heap = NULL, *data, **t4nodes = NULL;
	intptr_t i, ex, lex, *keys = NULL;;

	fprintf(stderr, "TEST 4 - INSERT + DEL_MIN\n");

	test_time(TIME_START);

	fprintf(stderr, "Setup: First inserting %ld nodes\n", count);

	// Setup phase
	if ((t4nodes = (void **)calloc(count, sizeof(void *))) == NULL) {
		fprintf(stderr, "Test 2 FAILED - Out of memory\n");
		test_time(TIME_SETUP);
		test_time(TIME_DONE);
		goto t4cleanup;
	}
	if((keys = (intptr_t *)calloc(count, sizeof(intptr_t))) == NULL) {
		fprintf(stderr, "Test 4 FAILED - Out of memory\n");
		test_time(TIME_SETUP);
		test_time(TIME_DONE);
		goto t4cleanup;
	}
	if((heap = pheap_create(NULL)) == NULL) {
		fprintf(stderr, "Test 4 FAILED - Unable to acquire a heap\n");
		test_time(TIME_SETUP);
		test_time(TIME_DONE);
		goto t4cleanup;
	}
	for(i = 0; i < count; i++) {
		ex = (intptr_t)random() % INTPTR_MAX;
		keys[i] = ex;
		t4nodes[i] = pheap_insert(heap, (void *)ex, (void *)ex);
	}
	fprintf(stderr, "Test 4 SETUP - Inserted %ld nodes\n", i);
	test_time(TIME_SETUP);

	// Issue a del_min followed by an insert count times
	fprintf(stderr, "Run: Now issuing a del_min followed by an insert %ld times\n", count);
	for(i = 0; i < count; i++) {
		pheap_delete_min(heap, NULL, NULL);
		ex = (intptr_t)random() % INTPTR_MAX;
		keys[i] = ex;
		t4nodes[i] = pheap_insert(heap, (void *)ex, (void *)ex);
	}
	fprintf(stderr, "Test 4 DONE - Ran del_min + random insert %ld times\n", i);
	test_time(TIME_DONE);

	// Delete/sort validation
	test_time(TIME_START);
	fprintf(stderr, "Test 4 - Now Sorting and Deleting\n");
	test_time(TIME_SETUP);
	i = ex = lex = 0;
	while(pheap_delete_min(heap, NULL, &data)) {
		ex = (intptr_t)data;
		if(ex < lex)
			break;
		lex = ex;
		i++;
	}
	fprintf(stderr, "Test 4 DONE - Sorted and Deleted %ld nodes\n", i);
	test_time(TIME_DONE);

	// Validate
	if(i < count)
		fprintf(stderr, "Test 4 FAILED - Out of order after  %ld deletions\n", i);
	else if (pheap_delete_min(heap, NULL, NULL))
		fprintf(stderr, "Test 4 FAILED - Tree is not empty\n");
	else
		fprintf(stderr, "Test 4 PASSED\n");

	// Cleanup
t4cleanup:
	if (t4nodes) {
		free(t4nodes);
		t4nodes = NULL;
	}
	if (keys) {
		free(keys);
		keys = NULL;
	}
	if (heap) {
		pheap_destroy(heap, NULL);
		heap = NULL;
	}
} // test4


void
test5(intptr_t count)
{
	void *heap = NULL, **t5nodes = NULL;
	intptr_t i, ex, lex, *keys = NULL;

	fprintf(stderr, "TEST 5 - FULL HEAP DESTROY\n");

	test_time(TIME_START);

	fprintf(stderr, "Setup: First inserting %ld nodes\n", count);

	t5nodes = (void **)calloc(count, sizeof(void *));
	if(t5nodes == NULL) {
		fprintf(stderr, "Test 2 FAILED - Out of memory\n");
		test_time(TIME_SETUP);
		test_time(TIME_DONE);
		goto t5cleanup;
	}

	keys = (intptr_t *)calloc(count, sizeof(intptr_t));
	if(keys == NULL) {
		fprintf(stderr, "Test 2 FAILED - Out of memory\n");
		test_time(TIME_SETUP);
		test_time(TIME_DONE);
		goto t5cleanup;
	}

	heap = pheap_create(NULL);

	for(i = 0; i < count; i++) {
		ex = (intptr_t)random() % INTPTR_MAX;
		keys[i] = ex;
		t5nodes[i] = pheap_insert(heap, (void *)ex, (void *)ex);
	}

	fprintf(stderr, "Test 5 SETUP - Inserted %ld nodes\n", i);

	// Issue a del_min followed by an insert count times

	fprintf(stderr, "Run: Now issuing a del_min followed by an insert %ld times\n", count);
	// Now Change all the keys
	for(i = 0; i < count; i++) {
		pheap_delete_min(heap, NULL, NULL);
		ex = (intptr_t)random() % INTPTR_MAX;
		keys[i] = ex;
		t5nodes[i] = pheap_insert(heap, (void *)ex, (void *)ex);
	}

	fprintf(stderr, "Test 5 SETUP - Ran del_min + random insert %ld times\n", i);
	test_time(TIME_SETUP);

	fprintf(stderr, "Test 5 RUN - Destroying heap of %ld nodes\n", i);

	pheap_destroy(heap, NULL);
	heap = NULL;

	test_time(TIME_DONE);

	fprintf(stderr, "Test 5 DONE - Destroyed Heap with %ld nodes\n", i);

	fprintf(stderr, "Test 5 PASSED\n");

	// Cleanup
t5cleanup:
	if (t5nodes) {
		free(t5nodes);
		t5nodes = NULL;
	}
	if (keys) {
		free(keys);
		keys = NULL;
	}
	if (heap) {
		pheap_destroy(heap, NULL);
		heap = NULL;
	}
} // test5


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

	test1(count);
	fprintf(stderr, "\n");
	test2(count);
	fprintf(stderr, "\n");
	test3(count);
	fprintf(stderr, "\n");
	test4(count);
	fprintf(stderr, "\n");
	test5(count);
} // main
