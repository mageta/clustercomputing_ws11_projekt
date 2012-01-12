#ifndef CONSTVECTOR_H
#define CONSTVECTOR_H

#include <stdio.h>
#include <string.h>

#include "vector.h"

struct constvector {
	vector_type * memchunks;

	size_t len; /* the length of the allocated memory for values */
	size_t elements; /* the actual count of managed elements */
	size_t element_size;

	/*
	 * Can be set and is then used to compare two elements of the vectors
	 * type.
	 * If it is NULL, then the elements are compared based on the
	 * memory (actually, memcmp() is used).
	 *
	 * = 0 - equal
	 * > 0 - greater
	 * < 0 - smaller
	 */
	int (* compare)(const void * lh, const void * rh, size_t element_size);
};

#ifndef CONSTVECTOR_TYPE
#define CONSTVECTOR_TYPE
	typedef struct constvector constvector_type;
#endif

int constvector_create(constvector_type ** vec, size_t len, size_t element_size)
	__attribute__((warn_unused_result));
void constvector_destroy(constvector_type * vec);

void * constvector_element(constvector_type *vec, unsigned int i)
	__attribute__((warn_unused_result));
int constvector_get(constvector_type *vec, unsigned int i, void * value);

void constvector_set(constvector_type *vec, unsigned int i, void * value);

int constvector_add(constvector_type *vec, void * value)
	__attribute__((warn_unused_result));
int constvector_del(constvector_type *vec, unsigned int i);

 int constvector_contains(constvector_type *vec, void * value)
 	__attribute__((warn_unused_result));

#define for_all_constvector_elements(vec, i) \
	for ((i) = 0; (i) < (vec)->elements; (i)++)

#endif // CONSTVECTOR_H
