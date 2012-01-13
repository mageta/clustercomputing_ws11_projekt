#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "stack.h"

#define TEST_NUMBERS 100
#define TEST_INT_FIRST 100

int
main(int argc, char ** argv)
{
	int i, rc;
	int numbers[TEST_NUMBERS], number;
	stack_type * test_stack;

	srand(time(NULL));

	for (i = 0; i < TEST_NUMBERS; i++) {
		numbers[i] = rand() % (TEST_NUMBERS * 10) + 1;
	}

	rc = stack_create(&test_stack, sizeof(*numbers));
	if(rc) {
		fprintf(stderr, "couldn't create a stack.. %s\n", strerror(rc));
		return rc;
	}

	for (i = 0; i < TEST_INT_FIRST; i++) {
		rc = stack_push(test_stack, &numbers[i]);
		if(rc) {
			fprintf(stderr, "couldn't push a new item.. %s\n",
					strerror(rc));
			goto err_free_list;
		}

		if(stack_size(test_stack) != (i + 1)) {
			fprintf(stderr, "size doesn't match up\n");
			goto err_free_list;
		}
	}

	while(stack_size(test_stack)) {
		rc = stack_pop(test_stack, &number);
		if(rc) {
			fprintf(stderr, "couldn't get a item.. %s\n", strerror(rc));
			goto err_free_list;
		}

		if(number != numbers[i - 1]) {
			fprintf(stderr, "recved items doesn't match\n");
			goto err_free_list;
		}

		i--;
	}

	if(i != 0) {
		fprintf(stderr, "size doesn't match up\n");
		goto err_free_list;
	}

	stack_destroy(test_stack);

	return 0;
err_free_list:
	stack_destroy(test_stack);
	return (rc ? rc : -1);
}
