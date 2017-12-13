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
test1(int count)
{
	void *oph;
	void *data;
	struct heap *root = NULL, *s;
	int i;
	uint64_t ex, lex;

	fprintf(stderr, "TEST 1 - SORTING\n");
	test_time(TIME_START);

	oph = pheap_create(NULL);
	for(i = 0; i < count; i++) {
		ex = (uint64_t)random();
		pheap_insert(oph, (void *)ex, (void *)ex);
	}

	test_time(TIME_SETUP);

	ex = 0;
	lex = 0;
	if((data = pheap_del_min(oph)))
		lex = (uint64_t)data;
	while((data = pheap_del_min(oph))) {
		ex = (uint64_t)data;
		if(ex < lex)
			break;
		lex = ex;
	}

	test_time(TIME_DONE);

	if(ex < lex) {
		fprintf(stderr, "Test 1 FAILED - Sort out of order\n");
	} else {
		fprintf(stderr, "Test 1 PASSED - Sort is in order\n");
	}

	// Cleanup
	pheap_destroy(oph);
} // test1


void
test2(int count)
{
	void *oph;
	void *data;
	struct heap *root = NULL, *s;
	int i, pos, cnt;
	uint64_t ex, lex;
	void **t2nodes = NULL;
	uint64_t *keys = NULL;

	fprintf(stderr, "TEST 2 - DELETE IN PLACE\n");

	test_time(TIME_START);
	t2nodes = (void **)calloc(sizeof(void *), count);
	if(t2nodes == NULL) {
		fprintf(stderr, "Test 2 FAILED - Out of memory\n");
		test_time(TIME_SETUP);
		test_time(TIME_DONE);
		return;
	}

	keys = (uint64_t *)calloc(sizeof(uint64_t), count);
	if(keys == NULL) {
		fprintf(stderr, "Test 2 FAILED - Out of memory\n");
		test_time(TIME_SETUP);
		test_time(TIME_DONE);
		return;
	}

	oph = pheap_create(NULL);

	for(i = 0; i < count; i++) {
		ex = (uint64_t)random();
		keys[i] = ex;
		t2nodes[i] = pheap_insert(oph, (void *)ex, (void *)ex);
	}

	fprintf(stderr, "Test 2 SETUP - Inserted %d nodes\n", i);

	test_time(TIME_SETUP);

	for(i = 0; i < count; i++) {
		ex =  (uint64_t)pheap_del(oph, t2nodes[i]);
//		fprintf(stderr, "ex = %llu, keys[i] = %llu\n", (unsigned long long)ex, (unsigned long long)keys[i]);
		if(ex != keys[i])
			break;
	}
	data = pheap_del_min(oph);

	fprintf(stderr, "Test 2 DONE - Deleted %d nodes\n", i);

	test_time(TIME_DONE);

	if(i < count)
		fprintf(stderr, "Test 2 FAILED - Mismatched keys after %d deletions\n", i);
	else if(data)
		fprintf(stderr, "Test 2 FAILED - Tree is not empty\n");
	else
		fprintf(stderr, "Test 2 PASSED\n");

	// Cleanup
	free(t2nodes);
	free(keys);
	pheap_destroy(oph);
} // test2


void
test3(int count)
{
	void *oph;
	void *data;
	struct heap *root = NULL, *s;
	int i, pos, cnt;
	uint64_t ex, lex;
	void **t3nodes = NULL;
	uint64_t *keys = NULL;

	fprintf(stderr, "TEST 3 - DECREASE(CHANGE) KEY and SORT\n");

	test_time(TIME_START);
	t3nodes = (void **)calloc(sizeof(void *), count);
	if(t3nodes == NULL) {
		fprintf(stderr, "Test 2 FAILED - Out of memory\n");
		test_time(TIME_SETUP);
		test_time(TIME_DONE);
		return;
	}

	keys = (uint64_t *)calloc(sizeof(uint64_t), count);
	if(keys == NULL) {
		fprintf(stderr, "Test 2 FAILED - Out of memory\n");
		test_time(TIME_SETUP);
		test_time(TIME_DONE);
		return;
	}

	oph = pheap_create(NULL);

	for(i = 0; i < count; i++) {
		ex = (uint64_t)random();
		keys[i] = ex;
		t3nodes[i] = pheap_insert(oph, (void *)ex, (void *)ex);
	}

	fprintf(stderr, "Test 3 SETUP - Inserted %d nodes\n", i);

	test_time(TIME_SETUP);

	// Now decrease all the keys
	for(i = 0; i < count; i++) {
		// Ensure it's a decrease key operation (not increasing)
		ex = keys[i] - (random() % keys[i]);
		keys[i] = ex;
		t3nodes[i] = pheap_decrease_key(oph, t3nodes[i], (void *)ex);
		pheap_set_data(t3nodes[i], (void *)ex);
	}

	fprintf(stderr, "Test 3 DONE - Changed Keys of %d nodes\n", i);
	test_time(TIME_DONE);

	if(i < count) {
		fprintf(stderr, "Test 3 FAILED - Mismatched keys after %d deletions\n", i);
		goto t3cleanup;
	}

	test_time(TIME_START);
	fprintf(stderr, "Test 3 - Sorting AND Deleting\n");
	test_time(TIME_SETUP);
	// Now delete
	ex = 0;
	lex = 0;
	i = 1;
	if((data = pheap_del_min(oph)))
		lex = (uint64_t)data;
	while((data = pheap_del_min(oph))) {
		ex = (uint64_t)data;
		if(ex < lex)
			break;
		lex = ex;
		i++;
	}
	test_time(TIME_DONE);

	fprintf(stderr, "Test 3 DONE - Sorted and Deleted %d nodes\n", i);

	data = pheap_del_min(oph);

	if(i < count)
		fprintf(stderr, "Test 3 FAILED - Out of order after  %d deletions\n", i);
	else if(data)
		fprintf(stderr, "Test 3 FAILED - Tree is not empty\n");
	else
		fprintf(stderr, "Test 3 PASSED\n");

	// Cleanup
t3cleanup:
	free(t3nodes);
	free(keys);
	pheap_destroy(oph);
} // test3


void
test4(int count)
{
	void *oph;
	void *data;
	struct heap *root = NULL, *s;
	int i, pos, cnt;
	uint64_t ex, lex;
	void **t4nodes = NULL;
	uint64_t *keys = NULL;

	fprintf(stderr, "TEST 4 - INSERT + DEL_MIN\n");

	test_time(TIME_START);

	fprintf(stderr, "Setup: First inserting %d nodes\n", count);

	t4nodes = (void **)calloc(sizeof(void *), count);
	if(t4nodes == NULL) {
		fprintf(stderr, "Test 2 FAILED - Out of memory\n");
		test_time(TIME_SETUP);
		test_time(TIME_DONE);
		return;
	}

	keys = (uint64_t *)calloc(sizeof(uint64_t), count);
	if(keys == NULL) {
		fprintf(stderr, "Test 2 FAILED - Out of memory\n");
		test_time(TIME_SETUP);
		test_time(TIME_DONE);
		return;
	}

	oph = pheap_create(NULL);

	for(i = 0; i < count; i++) {
		ex = (uint64_t)random();
		keys[i] = ex;
		t4nodes[i] = pheap_insert(oph, (void *)ex, (void *)ex);
	}

	fprintf(stderr, "Test 4 SETUP - Inserted %d nodes\n", i);

	test_time(TIME_SETUP);

	// Issue a del_min followed by an insert count times

	fprintf(stderr, "Run: Now issuing a del_min followed by an insert %d times\n", count);
	// Now Change all the keys
	for(i = 0; i < count; i++) {
		data = pheap_del_min(oph);
		ex = (uint64_t)random();
		keys[i] = ex;
		t4nodes[i] = pheap_insert(oph, (void *)ex, (void *)ex);
	}

	fprintf(stderr, "Test 4 DONE - Ran del_min + random insert %d times\n", i);
	test_time(TIME_DONE);

	test_time(TIME_START);
	fprintf(stderr, "Test 4 - Now Sorting and Deleting\n");
	test_time(TIME_SETUP);
	// Now delete
	ex = 0;
	lex = 0;
	i = 1;
	if((data = pheap_del_min(oph)))
		lex = (uint64_t)data;
	while((data = pheap_del_min(oph))) {
		ex = (uint64_t)data;
		if(ex < lex)
			break;
		lex = ex;
		i++;
	}
	test_time(TIME_DONE);

	fprintf(stderr, "Test 4 DONE - Sorted and Deleted %d nodes\n", i);

	data = pheap_del_min(oph);

	if(i < count)
		fprintf(stderr, "Test 4 FAILED - Out of order after  %d deletions\n", i);
	else if(data)
		fprintf(stderr, "Test 4 FAILED - Tree is not empty\n");
	else
		fprintf(stderr, "Test 4 PASSED\n");

	// Cleanup
t4cleanup:
	free(t4nodes);
	free(keys);
	pheap_destroy(oph);
} // test3


int
main(int argc, char *argv[])
{
	int count;
	if(argc != 2) {
		fprintf(stderr, "Usage: %s count\n", argv[0]);
		return 0;
	}
	count = atoi(argv[1]);
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
} // main
