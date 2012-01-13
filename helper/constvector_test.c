#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "constvector.h"

#define TEST_NUMBERS 100
#define TEST_INT_FIRST 100

//#define TEST_NUMBERS 6
//#define TEST_INT_FIRST 2

int
main(int argc, char ** argv)
{
	int i, rc;
	int numbers[TEST_NUMBERS], number, *num_p;
	constvector_type * test_vector;

	srand(time(NULL));

	for (i = 0; i < TEST_NUMBERS; i++) {
		numbers[i] = rand() % (TEST_NUMBERS * 10) + 1;
	}

	fprintf(stdout, "[create]\n");
	rc = constvector_create(&test_vector, 0, sizeof(*numbers));
	if(rc) {
		fprintf(stderr, "could not create the vector.. %s\n",
				strerror(rc));
		return rc;
	}

	fprintf(stdout, "[add]\n");
	for (i = 0; i < TEST_INT_FIRST; i++) {
		rc = constvector_add(test_vector, &numbers[i]);
		if(rc) {
			fprintf(stderr, "could not append to the vector.. %s\n",
					strerror(rc));
			goto err_free_vector;
		}
	}

	fprintf(stdout, "[get,element]\n");
	for (i = 0; i < TEST_INT_FIRST; i++) {
		rc = constvector_get(test_vector, i, &number);
		if(rc) {
			fprintf(stderr, "vector_copy_value failed.. %s\n",
					strerror(rc));
			goto err_free_vector;
		}

		if(number != numbers[i]) {
			fprintf(stderr, "constvector_get supplied a wrong"
					" value\n");
			goto err_free_vector;
		}

		num_p = constvector_element(test_vector, i);
		if(!num_p || (*num_p != numbers[i])) {
			fprintf(stderr, "constvector_element supplied a wrong"
					" value\n");
			goto err_free_vector;
		}
	}

	fprintf(stdout, "[compare]\n");
	for (i = 0; i < TEST_INT_FIRST; i++) {
		rc = constvector_contains(test_vector, &numbers[i]);
		if(!rc) {
			fprintf(stderr, "constvector_contains failed.\n");
			goto err_free_vector;
		}
	}

	fprintf(stdout, "[destroy]\n");
	constvector_destroy(test_vector);

	return 0;
err_free_vector:
	constvector_destroy(test_vector);
	return (rc ? rc : -1);
}
