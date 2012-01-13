#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "vector.h"

//#define TEST_NUMBERS 100
//#define TEST_INT_FIRST 100

#define TEST_NUMBERS	10
#define TEST_INT_FIRST	10

int
main(int argc, char ** argv)
{
	int i, rc;
	int numbers[TEST_NUMBERS], number;
	vector_type * test_vector;

	srand(time(NULL));

	for (i = 0; i < TEST_NUMBERS; i++) {
		numbers[i] = rand() % (TEST_NUMBERS * 10) + 1;
	}

	fprintf(stdout, "[create]\n");
	rc = vector_create(&test_vector, 0, sizeof(*numbers));
	if(rc) {
		fprintf(stderr, "could not create the vector.. %s\n",
				strerror(rc));
		return rc;
	}

	fprintf(stdout, "[add]\n");
	for (i = 0; i < TEST_INT_FIRST; i++) {
		rc = vector_add_value(test_vector, &numbers[i]);
		if(rc) {
			fprintf(stderr, "could not append to the vector.. %s\n",
					strerror(rc));
			goto err_free_vector;
		}
	}

	fprintf(stdout, "[values]\n");
	for (i = 0; i < TEST_INT_FIRST; i++) {
		rc = vector_copy_value(test_vector, i, &number);
		if(rc) {
			fprintf(stderr, "vector_copy_value failed.. %s\n",
					strerror(rc));
			goto err_free_vector;
		}

		if(number != numbers[i]) {
			fprintf(stderr, "vector_get supplied a wrong value\n");
			goto err_free_vector;
		}
	}

	fprintf(stdout, "[del]\n");
	for(i = 0; i < TEST_INT_FIRST; i++) {
		rc = vector_del_value(test_vector, 0);
		if(rc) {
			fprintf(stderr, "vector_del_value failed.. %s\n",
					strerror(rc));
			goto err_free_vector;
		}
	}

	if(test_vector->elements != 0) {
		fprintf(stderr, "number of elements doesn't count up\n");
		goto err_free_vector;
	}

	fprintf(stdout, "[add]\n");
	for (i = 0; i < TEST_INT_FIRST; i++) {
		rc = vector_add_value(test_vector, &i);
		if(rc) {
			fprintf(stderr, "could not append to the vector.. %s\n",
					strerror(rc));
			goto err_free_vector;
		}
	}

	fprintf(stdout, "[containes sorted]\n");
	for (i = 0; i < TEST_INT_FIRST; i++) {
		rc = vector_contains_sorted(test_vector, &i);
		if(!rc) {
			fprintf(stderr, "contain_sorted failed\n");
			goto err_free_vector;
		}
	}

	for (i = 0; i < TEST_INT_FIRST; i++) {
		number = *((int *) vector_get_value(test_vector, i));
		fprintf(stderr, "%d, ", number);
	}
	fprintf(stderr, "\n", number);

	vector_destroy(test_vector);

	return 0;
err_free_vector:
	vector_destroy(test_vector);
	return (rc ? rc : -1);
}
