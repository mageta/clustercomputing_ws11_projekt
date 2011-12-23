#include "vector.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char ** argv)
{
	vector_type *vec;

	int test_data[] = {
		63, 38, 61, 64, 24, 02, 46, 66, 55, 34, 25, 40, 45, 89, 29,
		03, 75, 99, 33, 68, 41, 80, 22, 11, 97, 47, 67, 12, 68, 99,
		79, 13, 43, 74, 56, 46, 51, 93, 84, 79, 84, 91, 71, 61, 82,
		12, 56, 67, 53, 72, 49, 92, 94, 51, 65, 79, 48, 88, 05, 82,
		69, 54, 70, 05, 38, 78, 75, 26, 58, 75, 61, 42, 69, 16, 39,
		77, 38, 46, 96, 38, 55, 89, 13, 48, 66, 87, 62, 52, 81, 25,
		43, 81, 64, 58, 46, 36, 77, 45, 16, 50
	};
	int test_size = 100;
	int i, rc;

	rc = vector_create(&vec, 0, sizeof(int));

	if(rc || !vec) {
		fprintf(stderr, "create failed: %s\n", strerror(rc));
		return -1;
	}

	for (i = 0; i < test_size; i++) {
		rc = vector_add_value(vec, &test_data[i]);
		if(rc) {
			fprintf(stderr, "add failed: %s\n", strerror(rc));
			return -1;
		}

		if(vec->elements != (i + 1)) {
			fprintf(stderr, "sice doesn't match\n");
			return -1;
		}
	}

	vector_destroy(vec);

	return 0;
}
