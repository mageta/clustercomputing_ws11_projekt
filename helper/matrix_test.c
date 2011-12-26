#include "matrix.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TEST_M 100
#define TEST_N 100

int
main(int argc, char ** argv)
{
	int numbers[TEST_M][TEST_N];
	int i, j, rc;
	matrix_type *test_matrix, *test_matrix_2;

	srand(time(NULL));

	for(i = 0; i < TEST_M; i++) {
		for(j = 0; j < TEST_N; j++) {
			numbers[i][j] = rand() % (TEST_M * TEST_N * 100);
		}
	}

	rc = matrix_create(&test_matrix, TEST_M, TEST_N, sizeof(*numbers));
	if(rc) {
		fprintf(stderr, "couldn't create a matrix.. %s\n", strerror(rc));
		return rc;
	}

	for(i = 0; i < TEST_M; i++) {
		for(j = 0; j < TEST_N; j++) {
			rc = matrix_set(test_matrix, i, j, &numbers[i][j]);
			if(rc) {
				fprintf(stderr, "couldn't set a value.. %s\n", strerror(rc));
				goto err_free_matrix;
			}
		}
	}

	for(i = 0; i < TEST_M; i++) {
		for(j = 0; j < TEST_N; j++) {
			if(!matrix_index_valid(test_matrix, i, j)) {
				fprintf(stderr, "dimensions doesn't match up\n");
				goto err_free_matrix;
			}

			if(numbers[i][j] != *((int *) matrix_get(test_matrix, i, j))) {
				fprintf(stderr, "values doesn't match up\n");
				goto err_free_matrix;
			}
		}
	}

	rc = matrix_copy(&test_matrix_2, test_matrix);
	if(rc) {
		fprintf(stderr, "coudln't copy a matrix.. %s\n", strerror(rc));
		goto err_free_matrix;
	}

	for(i = 0; i < TEST_M; i++) {
		for(j = 0; j < TEST_N; j++) {
			if(!matrix_index_valid(test_matrix_2, i, j)) {
				fprintf(stderr, "dimensions doesn't match up\n");
				goto err_free_matrix_2;
			}

			if(numbers[i][j] != *((int *) matrix_get(test_matrix_2, i, j))) {
				fprintf(stderr, "values doesn't match up\n");
				goto err_free_matrix_2;
			}
		}
	}

	matrix_init(test_matrix_2, 2);

	for(i = 0; i < TEST_M; i++) {
		for(j = 0; j < TEST_N; j++) {
			int value = *((int *) matrix_get(test_matrix_2, i, j));
			if(value != 2) {
				fprintf(stderr, "values doesn't match up\n"
						"Should be %d bit is %d (%#x)\n",
						2, value, value);
				goto err_free_matrix_2;
			}
		}
	}

	matrix_destroy(test_matrix_2);
	matrix_destroy(test_matrix);

	return 0;
err_free_matrix_2:
	matrix_destroy(test_matrix_2);
err_free_matrix:
	matrix_destroy(test_matrix);
	return (rc ? rc : -1);
}
