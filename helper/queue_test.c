#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "queue.h"
#include "list.h"

#define TEST_NUMBERS 100
#define TEST_INT_FIRST 100

int
main(int argc, char ** argv)
{
	int i, rc;
	int numbers[TEST_NUMBERS], number;
	queue_type * test_queue;

	srand(time(NULL));

	for (i = 0; i < TEST_NUMBERS; i++) {
		numbers[i] = rand() % (TEST_NUMBERS * 10) + 1;
	}

	rc = queue_create(&test_queue, sizeof(*numbers));
	if(rc) {
		fprintf(stderr, "couldn't create a queue.. %s\n", strerror(rc));
		return rc;
	}

	for (i = 0; i < TEST_INT_FIRST; i++) {
		rc = queue_enqueue(test_queue, &numbers[i]);
		if(rc) {
			fprintf(stderr, "couldn't enqueue a new item.. %s\n",
					strerror(rc));
			goto err_free_list;
		}

		if(queue_size(test_queue) != (i + 1)) {
			fprintf(stderr, "size doesn't match up\n");
			goto err_free_list;
		}
	}

	while(queue_size(test_queue)) {
		rc = queue_dequeue(test_queue, &number);
		if(rc) {
			fprintf(stderr, "couldn't get a item.. %s\n", strerror(rc));
			goto err_free_list;
		}

		if(number != numbers[TEST_INT_FIRST - i]) {
			fprintf(stderr, "recved items doesn't match\n");
			goto err_free_list;
		}

		i--;
	}

	if(i != 0) {
		fprintf(stderr, "size doesn't match up\n");
		goto err_free_list;
	}

	queue_destroy(test_queue);

	return 0;
err_free_list:
	queue_destroy(test_queue);
	return (rc ? rc : -1);
}
