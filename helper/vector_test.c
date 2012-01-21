#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "vector.h"
#include "algorithm.h"

#define TEST_NUMBERS 100
#define TEST_INT_FIRST 100

// #define TEST_NUMBERS	10
// #define TEST_INT_FIRST	10

void __output(vector_type * vec)
{
	unsigned int i;
	int *num_p;

	for(i = 0; i < (vec->elements - 1); i++) {
		num_p = (int *) vector_get_value(vec, i);
		if(!num_p)
			continue;
		fprintf(stdout, "%d, ", *num_p);
	}
	num_p = (int *) vector_get_value(vec, i);
	if(!num_p) {
		fprintf(stdout, "\n");
		return;
	}
	fprintf(stdout, "%d\n", *num_p);
}

int intcompare(const void *lh, const void *rh, size_t size)
{
	int lhv = *((int *) lh);
	int rhv = *((int *) rh);

	if(lhv < rhv)
		return -1;
	else if(lhv > rhv)
		return 1;
	else
		return 0;
}

int
main(int argc, char ** argv)
{
	int i, rc, max;
	int numbers[TEST_NUMBERS], number, *num_p;
	unsigned int pos;
	vector_type * test_vector;
	vector_type * test_vector2;

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
	test_vector->compare = intcompare;

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

	fprintf(stdout, "[insert get]\n");
	for(i = 1; i < (TEST_INT_FIRST + 1); i++) {
		rc = vector_insert(test_vector, i + (1 * (i - 1)), &i);
		if(rc) {
			fprintf(stderr, "vector_insert failed.. %s\n",
					strerror(rc));
			goto err_free_vector;
		}

		num_p = (int *) vector_get_value(test_vector,
				i + (1 * (i - 1)));
		if(!num_p || *num_p != i) {
			fprintf(stderr, "vector_insert failed, wrong value\n");
			goto err_free_vector;
		}
	}

	fprintf(stdout, "[del]\n");
	for(i = 0; i < (TEST_INT_FIRST * 2); i++) {
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

	fprintf(stdout, "[is_sorted]\n");
	rc = vector_is_sorted(test_vector);
	if(!rc) {
		fprintf(stderr, "is_sorted failed\n");
		goto err_free_vector;
	}

	fprintf(stdout, "[containes sorted]\n");
	for (i = 0; i < TEST_INT_FIRST; i++) {
		num_p = (int *) bsearch_vector(test_vector, &i, NULL);
		if(!num_p || *num_p != i) {
			fprintf(stderr, "contain_sorted failed\n");
			goto err_free_vector;
		}
	}

	fprintf(stdout, "[create]\n");
	rc = vector_create(&test_vector2, 0, sizeof(*numbers));
	if(rc) {
		fprintf(stderr, "could not create a vector.. %s\n",
				strerror(rc));
		goto err_free_vector;
	}
	test_vector2->compare = intcompare;

	fprintf(stdout, "[add]\n");
	for (i = 0, max = 0; i < TEST_INT_FIRST; i++) {
		do {
			number = rand() % TEST_INT_FIRST;
		} while (number < max);
		max = number;

		rc = vector_add_value(test_vector2, &number);
		if(rc) {
			fprintf(stderr, "could not append to the vector.. %s\n",
					strerror(rc));
			goto err_free_vector2;
		}
	}

	fprintf(stdout, "[merge]\n");
	rc = merge_sorted_vector(test_vector, test_vector2);
	if(rc) {
		fprintf(stderr, "couldn't merge the two vectors.. %s\n",
				strerror(rc));
		goto err_free_vector2;
	}

	if(!vector_is_sorted(test_vector)) {
		fprintf(stderr, "merge failed\n");
		goto err_free_vector2;
	}

	for (i = 0; i < TEST_INT_FIRST; i++) {
		num_p = bsearch_vector(test_vector, &i, &pos);
		if(!num_p) {
			fprintf(stderr, "merge failed\n");
			goto err_free_vector2;
		}

		vector_del_value(test_vector, pos);
	}

	for (i = 0; i < TEST_INT_FIRST; i++) {
		number = *((int *) vector_get_value(test_vector2, i));
		num_p = bsearch_vector(test_vector, &number, &pos);
		if(!num_p) {
			fprintf(stderr, "merge failed\n");
			goto err_free_vector2;
		}

		vector_del_value(test_vector, pos);
	}

	if(test_vector->elements != 0) {
		fprintf(stderr, "number of elements doesn't count up\n");
		goto err_free_vector2;
	}

	fprintf(stdout, "[insert_sorted]\n");
	for (i = 0; i < TEST_INT_FIRST; i++) {
		number = rand() % (TEST_INT_FIRST * 10);

		rc = vector_insert_sorted(test_vector, &number, 1);
		if(rc) {
			fprintf(stderr, "could not insert into to vector.."
					" %s\n", strerror(rc));
			goto err_free_vector2;
		}

		rc = vector_is_sorted(test_vector);
		if(!rc) {
			fprintf(stderr, "insert ended up in a unsorted vector"
					"\n");
			__output(test_vector);
			goto err_free_vector2;
		}
	}

	fprintf(stdout, "[is_sorted]\n");
	number = *((int *) vector_get_value(test_vector, TEST_INT_FIRST - 1));
	number++;
	rc = vector_insert(test_vector, 0, &number);
	if(rc) {
		fprintf(stderr, "could not insert into to vector.."
				" %s\n", strerror(rc));
		goto err_free_vector2;
	}

	rc = vector_is_sorted(test_vector);
	if(rc) {
		fprintf(stderr, "is_sorted failed"
				"\n");
		__output(test_vector);
		goto err_free_vector2;
	}

	fprintf(stdout, "[del]\n");
	max = test_vector->elements;
	for(i = 0; i < max; i++) {
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

	for (i = 0; i < (TEST_INT_FIRST / 2); i++) {
		pos = test_vector->elements / 2;
		max = 10;
		rc = vector_insert(test_vector, pos, &max);
		if(rc) {
			fprintf(stderr, "could not append to the vector.. %s\n",
					strerror(rc));
			goto err_free_vector;
		}
	}

	vector_destroy(test_vector2);
	rc = vector_create(&test_vector2, 0, sizeof(number));
	if(rc) {
		fprintf(stdout, "could not create a vector.. %s\n",
				strerror(rc));
		goto err_free_vector;
	}
	test_vector2->compare = intcompare;

	rc = vector_massmove(test_vector, 5, 9, test_vector->elements - 1, NULL);
	if(rc) {
		fprintf(stdout, "massmove failed.. %s\n", strerror(rc));
		goto err_free_vector2;
	}
	__output(test_vector);

	fprintf(stdout, "[insert_sorted]\n");
	vector_destroy(test_vector2);
	rc = vector_create(&test_vector2, TEST_NUMBERS * 100, sizeof(number));
	if(rc) {
		fprintf(stdout, "could not create a vector.. %s\n",
				strerror(rc));
		goto err_free_vector;
	}
	test_vector2->compare = intcompare;

	for(i = 0; i < TEST_NUMBERS * 100; i++) {
		number = rand() % 1000;

		rc = vector_insert_sorted(test_vector2, &number, 1);
		if(rc) {
			fprintf(stderr, "could not insert into to vector.."
					" %s\n", strerror(rc));
			goto err_free_vector2;
		}

		rc = vector_is_sorted(test_vector2);
		if(!rc) {
			fprintf(stderr, "insert ended up in a unsorted vector"
					"\n");
			__output(test_vector2);
			goto err_free_vector2;
		}
	}

	vector_destroy(test_vector);
	vector_destroy(test_vector2);

	return 0;
err_free_vector2:
	vector_destroy(test_vector2);
err_free_vector:
	vector_destroy(test_vector);
	return (rc ? rc : -1);
}
