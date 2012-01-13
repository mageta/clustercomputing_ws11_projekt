#include "matrix.h"

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>

int
matrix_create(matrix_type **mat, size_t m, size_t n, size_t element_size)
{
	int rc = 0;
	matrix_type *new_m;

	if(!m || !n || !mat || !element_size)
		return EINVAL;

	new_m = (matrix_type *) malloc(sizeof(*new_m));
	if(!new_m)
		return ENOMEM;
	memset(new_m, 0, sizeof(*new_m));

	new_m->m = m;
	new_m->n = n;
	new_m->element_size = element_size;

	new_m->matrix = calloc(new_m->m * new_m->n, new_m->element_size);
	if(!new_m->matrix) {
		rc = ENOMEM;
		goto err_free;
	}

	*mat = new_m;

	return rc;
err_free:
	free(new_m);
	return rc;
}

void
matrix_destroy(matrix_type *m)
{
	if(!m)
		return;

	free(m->matrix);
	free(m);
}

int matrix_set(matrix_type *matrix, size_t i, size_t j, void * value)
{
	if(!matrix || !matrix->matrix || !value ||
			(i >= matrix->m) || (j >= matrix->n))
		return EINVAL;

	memcpy(((char *) matrix->matrix) +
			((i * matrix->n * matrix->element_size) +
			 (j * matrix->element_size)),
			value, matrix->element_size);

	return 0;
}

void * matrix_get(matrix_type *matrix, size_t i, size_t j)
{
	if(!matrix || !matrix->matrix || (i >= matrix->m) || (j >= matrix->n))
		return NULL;

	return (void *) (((char *) matrix->matrix) +
			((i * matrix->n * matrix->element_size) +
			 (j * matrix->element_size)));
}

size_t matrix_size(matrix_type *matrix)
{
	if(!matrix)
		return 0;

	return matrix->m * matrix->n;
}

size_t matrix_size_byte(matrix_type *matrix)
{
	if(!matrix)
		return 0;

	return matrix_size(matrix) * matrix->element_size;
}

void matrix_init(matrix_type *matrix, char c)
{
	int i, endian;
	char * element;
	size_t size;

	if(!matrix || !matrix->matrix)
		return;

	/* this function is a little nasty */

	size = matrix_size(matrix);
	element = matrix->matrix;

	endian = 1;
	if(*((char *) &endian) == 1)
		endian = 1; /* little endian */
	else
		endian = 0; /* big endian */

	if((c == 0) || (matrix->element_size == 1))
		memset(matrix->matrix, c, matrix_size_byte(matrix));
	else {
		for(i = 0; i < matrix_size(matrix);
				i++, element += matrix->element_size) {
			memset(element, 0, matrix->element_size);
			*(endian ? element : element + (matrix->element_size - 1))
				= c;
		}
	}
}

int
matrix_copy(matrix_type **new, matrix_type *old)
{
	int rc;
	matrix_type *new_m;

	if(!new || !old || !old->matrix)
		return EINVAL;

	rc = matrix_create(&new_m, old->m, old->n, old->element_size);
	if(rc)
		return rc;

	memcpy(new_m->matrix, old->matrix, matrix_size_byte(old));

	*new = new_m;

	return 0;
}

int matrix_index_valid(matrix_type *matrix, int i, int j)
{
	if(!matrix)
		return 0;

	return (i >= 0) && (i < matrix->m) && (j >= 0) && (j < matrix->n);
}
