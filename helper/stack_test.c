#include "stack.h"
#include "vector.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char ** argv)
{
	stack_type *stack;

	int test_data[] = {
		0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99
	};
	int test_size = 100;
	int i, rc, val;

	rc = stack_create(&stack, 0, sizeof(int));

	if(rc || !stack) {
		fprintf(stderr, "create failed: %s\n", strerror(rc));
		return -1;
	}

	for (i = 0; i < test_size; i++) {
		rc = stack_push(stack, &test_data[i]);
		if(rc) {
			fprintf(stderr, "add failed: %s\n", strerror(rc));
			return -1;
		}

		if(stack_size(stack) != (i + 1)) {
			fprintf(stderr, "sice doesn't match\n");
			return -1;
		}
	}

	while(stack_size(stack)) {
		i--;
		stack_pop(stack, &val);

		if(i != val) {
			fprintf(stderr, "values doesn't match up\n");
			return -1;
		}
	}

	stack_destroy(stack);

	return 0;
}
