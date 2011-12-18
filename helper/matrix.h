#ifndef MATRIX_H
#define MATRIX_H

#include <stdio.h>
#include <string.h>

#include <asm/errno.h>

struct matrix {
	size_t m, n;
	void * matrix;

	size_t element_size;
};

typedef struct matrix matrix_t;

int matrix_create(matrix_t **matrix, size_t m, size_t n, size_t element_size);
void matrix_destroy(matrix_t *matrix);

int matrix_set(matrix_t *matrix, size_t i, size_t j, void * value);
void * matrix_get(matrix_t *matrix, size_t i, size_t j);
size_t matrix_size(matrix_t *matrix);
size_t matrix_size_byte(matrix_t *matrix);
void matrix_init(matrix_t *matrix, int c);
int matrix_copy(matrix_t **new_matrix, matrix_t *old_matrix);

int matrix_index_valid(matrix_t *matrix, int i, int j);

#endif // MATRIX_H
