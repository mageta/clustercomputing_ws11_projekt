#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "list.h"
#include "algorithm.h"

#define TEST_NUMBERS 100
#define TEST_INT_FIRST 25
#define TEST_INT_SECOND 50
#define TEST_INT_THIRD 75

//#define TEST_NUMBERS 6
//#define TEST_INT_FIRST 2
//#define TEST_INT_SECOND 4
//#define TEST_INT_THIRD 6

void __output(list_type *list)
{
	int i;
	int * num_p;

	for(i = 0; i < (list->elements - 1); i++) {
		num_p = list_element(list, i);
		if(!num_p)
			continue;
		fprintf(stdout, "%d, ", *num_p);
	}
	num_p = list_element(list, i);
	if(!num_p) {
		fprintf(stdout, "\n");
		return;
	}
	fprintf(stdout, "%d\n", *num_p);
}

int
main(int argc, char ** argv)
{
	int i, rc;
	unsigned int pos;
	int numbers[TEST_NUMBERS], number, *num_p;
	list_type * test_list;

	srand(time(NULL));

	for (i = 0; i < TEST_NUMBERS; i++) {
		numbers[i] = rand() % (TEST_NUMBERS * 10) + 1;
	}

	fprintf(stdout, "[create]\n");
	rc = list_create(&test_list, sizeof(*numbers));
	if(rc) {
		fprintf(stderr, "could not create the list.. %s\n", strerror(rc));
		return rc;
	}

	fprintf(stdout, "[append]\n");
	for (i = 0; i < TEST_INT_FIRST; i++) {
		rc = list_append(test_list, &numbers[i]);
		if(rc) {
			fprintf(stderr, "could not append to the list.. %s\n",
					strerror(rc));
			goto err_free_list;
		}
	}

	fprintf(stdout, "[get]\n");
	for (i = 0; i < TEST_INT_FIRST; i++) {
		rc = list_get(test_list, i, &number);
		if(rc) {
			fprintf(stderr, "list_get failed.. %s\n", strerror(rc));
			goto err_free_list;
		}

		if(number != numbers[i]) {
			fprintf(stderr, "list_get supplied a wrong value\n");
			goto err_free_list;
		}
	}

	fprintf(stdout, "[prepend]\n");
	for (i = TEST_INT_FIRST; i < TEST_INT_SECOND; i++) {
		rc = list_prepend(test_list, &numbers[i]);
		if(rc) {
			fprintf(stderr, "could not prepend to the list.. %s\n",
					strerror(rc));
			goto err_free_list;
		}
	}

	fprintf(stdout, "[get]\n");
	for (i = 0; i < TEST_INT_FIRST; i++) {
		rc = list_get(test_list, i, &number);
		if(rc) {
			fprintf(stderr, "list_get failed.. %s\n", strerror(rc));
			goto err_free_list;
		}

		if(number != numbers[TEST_INT_SECOND - i - 1]) {
			fprintf(stderr, "list_get supplied a wrong value\n");
			goto err_free_list;
		}
	}

	for (; i < TEST_INT_SECOND; i++) {
		rc = list_get(test_list, i, &number);
		if(rc) {
			fprintf(stderr, "list_get failed.. %s\n", strerror(rc));
			goto err_free_list;
		}

		if(number != numbers[i - TEST_INT_FIRST]) {
			fprintf(stderr, "list_get supplied a wrong value\n");
			goto err_free_list;
		}
	}

	fprintf(stdout, "[insert]\n");
	for (i = TEST_INT_SECOND; i < TEST_INT_THIRD; i++) {
		rc = list_insert_at(test_list, i - TEST_INT_FIRST, &numbers[i]);
		if(rc) {
			fprintf(stderr, "could not insert into the list.. %s\n",
					strerror(rc));
			goto err_free_list;
		}
	}

	fprintf(stdout, "[get]\n");
	for (i = 0; i < TEST_INT_FIRST; i++) {
		rc = list_get(test_list, i, &number);
		if(rc) {
			fprintf(stderr, "list_get failed.. %s\n", strerror(rc));
			goto err_free_list;
		}

		if(number != numbers[TEST_INT_SECOND - i - 1]) {
			fprintf(stderr, "list_get supplied a wrong value\n");
			goto err_free_list;
		}
	}

	for (; i < TEST_INT_SECOND; i++) {
		rc = list_get(test_list, i, &number);
		if(rc) {
			fprintf(stderr, "list_get failed.. %s\n", strerror(rc));
			goto err_free_list;
		}

		if(number != numbers[i + TEST_INT_FIRST]) {
			fprintf(stderr, "list_get supplied a wrong value\n");
			goto err_free_list;
		}
	}

	for (; i < TEST_INT_THIRD; i++) {
		rc = list_get(test_list, i, &number);
		if(rc) {
			fprintf(stderr, "list_get failed.. %s\n", strerror(rc));
			goto err_free_list;
		}

		if(number != numbers[i - TEST_INT_SECOND]) {
			fprintf(stderr, "list_get supplied a wrong value\n");
			goto err_free_list;
		}
	}

	fprintf(stdout, "[remove]\n");
	for(i = TEST_INT_FIRST; i < TEST_INT_SECOND; i++) {
		rc = list_remove(test_list, TEST_INT_FIRST);
		if(rc) {
			fprintf(stderr, "list_remove failed.. %s\n", strerror(rc));
			goto err_free_list;
		}
	}

	fprintf(stdout, "[get]\n");
	for (i = 0; i < TEST_INT_FIRST; i++) {
		rc = list_get(test_list, i, &number);
		if(rc) {
			fprintf(stderr, "list_get failed.. %s\n", strerror(rc));
			goto err_free_list;
		}

		if(number != numbers[TEST_INT_SECOND - i - 1]) {
			fprintf(stderr, "list_get supplied a wrong value\n");
			goto err_free_list;
		}
	}

	for (; i < TEST_INT_SECOND; i++) {
		rc = list_get(test_list, i, &number);
		if(rc) {
			fprintf(stderr, "list_get failed.. %s\n", strerror(rc));
			goto err_free_list;
		}

		if(number != numbers[i - TEST_INT_FIRST]) {
			fprintf(stderr, "list_get supplied a wrong value\n");
			goto err_free_list;
		}
	}

	fprintf(stdout, "[remove_head]\n");
	for(i = 0; i < TEST_INT_FIRST; i++) {
		rc = list_remove_head(test_list);
		if(rc) {
			fprintf(stderr, "list_remove failed.. %s\n", strerror(rc));
			goto err_free_list;
		}
	}

	fprintf(stdout, "[get]\n");
	for (i = 0; i < TEST_INT_FIRST; i++) {
		rc = list_get(test_list, i, &number);
		if(rc) {
			fprintf(stderr, "list_get failed.. %s\n", strerror(rc));
			goto err_free_list;
		}

		if(number != numbers[i]) {
			fprintf(stderr, "list_get supplied a wrong value\n");
			goto err_free_list;
		}
	}

	fprintf(stdout, "[remove_tail]\n");
	for(i = 0; i < TEST_INT_FIRST; i++) {
		rc = list_remove_tail(test_list);
		if(rc) {
			fprintf(stderr, "list_remove failed.. %s\n", strerror(rc));
			goto err_free_list;
		}
	}

	if(test_list->elements > 0 || test_list->head || test_list->tail) {
		fprintf(stderr, "list should be empty, but is not\n");
		goto err_free_list;
	}

	fprintf(stdout, "[append]\n");
	for (i = 0; i < TEST_INT_FIRST; i++) {
		rc = list_append(test_list, &i);
		if(rc) {
			fprintf(stderr, "could not append to the list.. %s\n",
					strerror(rc));
			goto err_free_list;
		}
	}

	fprintf(stdout, "[bserach]\n");
	for (i = 0; i < TEST_INT_FIRST; i++) {
		num_p = bsearch_list(test_list, &i, &pos);
		if(!number || *num_p != i || pos != i) {
			fprintf(stderr, "could not append to the list.. %s\n",
					strerror(rc));
			goto err_free_list;
		}
	}

	fprintf(stdout, "[remove_tail]\n");
	for(i = 0; i < TEST_INT_FIRST; i++) {
		rc = list_remove_tail(test_list);
		if(rc) {
			fprintf(stderr, "list_remove failed.. %s\n", strerror(rc));
			goto err_free_list;
		}
	}

	if(test_list->elements > 0 || test_list->head || test_list->tail) {
		fprintf(stderr, "list should be empty, but is not\n");
		goto err_free_list;
	}

	fprintf(stdout, "[insert_sorted]\n");
	for (i = 0; i < TEST_INT_FIRST; i++) {
		number = rand() % (TEST_INT_FIRST * 10);

		rc = list_insert_sorted(test_list, &number);
		if(rc) {
			fprintf(stderr, "could not insert into the list.."
					" %s\n", strerror(rc));
			goto err_free_list;
		}

		rc = list_is_sorted(test_list);
		if(!rc) {
			fprintf(stderr, "insert ended up in a unsorted list\n");
			__output(test_list);
			goto err_free_list;
		}
	}

	fprintf(stdout, "[is_sorted]\n");
	number = *((int *) list_element(test_list, TEST_INT_FIRST - 1));
	number++;
	rc = list_insert_at(test_list, 0, &number);
	if(rc) {
		fprintf(stderr, "could not insert into the list.."
				" %s\n", strerror(rc));
		goto err_free_list;
	}

	rc = list_is_sorted(test_list);
	if(rc) {
		fprintf(stderr, "is_sorted failed"
				"\n");
		__output(test_list);
		goto err_free_list;
	}

	list_destroy(test_list);

	return 0;
err_free_list:
	list_destroy(test_list);
	return (rc ? rc : -1);
}
