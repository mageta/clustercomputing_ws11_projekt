#include "vector.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

int vector_create(vector_type ** v, size_t len, size_t element_size)
{
	vector_type * vec;

	if(!v || !element_size)
		return EINVAL;

	vec = (vector_type *) malloc(sizeof(*vec));
	if(!vec)
		return ENOMEM;
	memset(vec, 0, sizeof(*vec));

	vec->values = NULL;
	if(len >= 1) {
		vec->values = malloc(element_size * len);
		if(!vec->values)
			goto err_free;
		memset(vec->values, 0, element_size * len);
	}

	vec->len = len;
	vec->elements = 0;
	vec->element_size = element_size;

	*v = vec;

	return 0;
err_free:
	free(vec);
	return ENOMEM;
}

void vector_destroy(vector_type * vec)
{
	if(!vec)
		return;

	free(vec->values);
	free(vec);
}

void * vector_get_value(vector_type *vec, unsigned int i)
{
	if(!vec || !vec->values || (i >= vec->elements))
		return NULL;

	return (void *) (((char *) vec->values) + (i * vec->element_size));
}

void vector_set_value(vector_type *vec, unsigned int i, void * value)
{
	if(!vec || !vec->values || !value || (i >= vec->len))
		return;

	memcpy(((char *) vec->values) + (vec->elements * vec->element_size),
			value, vec->element_size);
}

int vector_copy_value(vector_type *vec, unsigned int i, void * value)
{
	if(!vec || !vec->values || !value || (i >= vec->elements))
		return EINVAL;

	memcpy(value,
		((char *) vec->values) + (i * vec->element_size),
		vec->element_size);
	return 0;
}

int vector_add_value(vector_type *vec, void * value)
{
	char * new_values;
	size_t old_len, new_len;

	if(!vec || !value)
		return EINVAL;

	if(!vec->values && (vec->len > 0))
		return EFAULT;

	old_len = vec->len;
	new_len = old_len * 2;

	if(vec->elements >= old_len) {
		if(new_len < 1)
			new_len = 1;

		new_values = realloc(vec->values, new_len * vec->element_size);
		if(!new_values)
			return ENOMEM;

		vec->len = new_len;
		vec->values = (void *) new_values;

		memset(new_values + (old_len * vec->element_size), 0,
				(new_len * vec->element_size) -
				(old_len * vec->element_size));
	}

	vector_set_value(vec, vec->elements, value);
	vec->elements++;

	return 0;
}

int vector_del_value(vector_type *vec, unsigned int i)
{
	char * values;

	if(!vec || !vec->values || (i >= vec->elements))
		return EINVAL;

	values = vec->values;

	memmove(values + (i * vec->element_size),
			values + ((i+1) * vec->element_size),
			(vec->elements - 1 - i) * vec->element_size);

	vec->elements -= 1;

	return 0;
}
