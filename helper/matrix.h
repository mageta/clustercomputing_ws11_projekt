#ifndef MATRIX_H
#define MATRIX_H

#include <stdio.h>
#include <string.h>

struct matrix {
	size_t m, n;
	void * matrix;

	size_t element_size;
};

typedef struct matrix matrix_type;

int matrix_create(matrix_type **matrix, size_t m, size_t n,
		size_t element_size);
void matrix_destroy(matrix_type *matrix);

int matrix_set(matrix_type *matrix, size_t i, size_t j, void * value);
void * matrix_get(matrix_type *matrix, size_t i, size_t j);
size_t matrix_size(matrix_type *matrix);
size_t matrix_size_byte(matrix_type *matrix);
void matrix_init(matrix_type *matrix, char c);
int matrix_copy(matrix_type **new_matrix, matrix_type *old_matrix);

int matrix_index_valid(matrix_type *matrix, int i, int j);

#endif // MATRIX_H
