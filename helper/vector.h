#ifndef VECTOR_H
#define VECTOR_H

#include <stdio.h>
#include <string.h>

struct vector {
	void * values;

	size_t len;
	size_t elements;
	size_t element_size;
};

typedef struct vector vector_type;

int vector_create(vector_type ** vec, size_t len, size_t element_size);
void vector_destroy(vector_type * vec);

void * vector_get_value(vector_type *vec, unsigned int i);
void vector_set_value(vector_type *vec, unsigned int i, void * value);
void vector_copy_value(vector_type *vec, unsigned int i, void * value);

int vector_add_value(vector_type *vec, void * value);
int vector_del_value(vector_type *vec, unsigned int i);

#define for_all_vector_elements(vec, i) \
	for ((i) = 0; (i) < (vec)->elements; (i)++)

#endif // VECTOR_H
