#ifndef VECTOR_H
#define VECTOR_H

#include <stdio.h>
#include <string.h>

#ifndef LIST_TYPE
#define LIST_TYPE
	typedef struct list list_type;
#endif
#ifndef CONSTVECTOR_TYPE
#define CONSTVECTOR_TYPE
	typedef struct constvector constvector_type;
#endif

struct vector {
	void * values;

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
	 *
	 * If a function compares a element of the vector with a external 'key',
	 * then the 'key' is always rh.
	 */
	int (* compare)(const void * lh, const void * rh, size_t element_size);
};


#ifndef VECTOR_TYPE
#define VECTOR_TYPE
	typedef struct vector vector_type;
#endif

int vector_create(vector_type ** vec, size_t len, size_t element_size)
	__attribute__((warn_unused_result));
void vector_destroy(vector_type * vec);

void * vector_get_value(vector_type *vec, unsigned int i)
	__attribute__((warn_unused_result));
void vector_set_value(vector_type *vec, unsigned int i, void * value);
int vector_copy_value(vector_type *vec, unsigned int i, void * value);

int vector_add_value(vector_type *vec, void * value)
	__attribute__((warn_unused_result));
int vector_del_value(vector_type *vec, unsigned int i);

int vector_insert(vector_type *vec, unsigned int i, void * value)
	__attribute__((warn_unused_result));
int vector_insert_sorted(vector_type *vec, void * value, int allow_doubles)
	__attribute__((warn_unused_result));
int vector_insert_sorted_pos(vector_type *vec, void * value, int allow_doubles, 
		unsigned int *rpos)
	__attribute__((warn_unused_result));

int vector_append_list(vector_type *vec, list_type *list);
int vector_append_constvector(vector_type *vec, constvector_type *cvec);
int vector_append_vector(vector_type *dest, vector_type *src);

int vector_contains(vector_type *vec, void * value)
	__attribute__((warn_unused_result));
int vector_is_sorted(vector_type *vec);

int vector_massmove(vector_type *vec, unsigned int from, unsigned int to,
	unsigned int newpos, vector_type *buf)
	__attribute__((warn_unused_result));

#define for_all_vector_elements(vec, i) \
	for ((i) = 0; (i) < (vec)->elements; (i)++)

#endif // VECTOR_H
