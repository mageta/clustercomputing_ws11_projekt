#include "vector.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include "list.h"
#include "constvector.h"

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
	vec->compare = memcmp;

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

	memcpy(((char *) vec->values) + (i * vec->element_size),
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

static int __vector_increase(vector_type *vec, size_t inc)
{
	char * new_values;
	int old_len = vec->len;
	int new_len = old_len + inc;

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

	return 0;
}

int vector_add_value(vector_type *vec, void * value)
{
	int rc = 0;

	if(!vec || !value)
		return EINVAL;

	if(!vec->values && (vec->len > 0))
		return EFAULT;

	if(vec->elements >= vec->len) {
		rc = __vector_increase(vec, vec->len);
		if(rc)
			return rc;
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

int __append_checks(vector_type *vec, size_t src_size)
{
	int rc = 0;

	if(!vec)
		return EINVAL;

	if(!vec->values && (vec->len > 0))
		return EFAULT;

	if((vec->elements + src_size) > vec->len) {
		/*
		 * TODO: think about a better strategy to increase
		 * the mem in this case
		 */
		rc = __vector_increase(vec, src_size);
		if(rc)
			return rc;
	}

	return 0;
}

int vector_append_list(vector_type *vec, list_type *list)
{
	int rc = 0, i;
	void *value;

	if(!list)
		return EINVAL;

	rc = __append_checks(vec, list->elements);
	if(rc)
		return rc;

	for(i = 0; i < list->elements; i++) {
		value = list_element(list, i);
		if(!value)
			return EFAULT;

		vector_set_value(vec, vec->elements, value);
		vec->elements++;
	}

	return 0;
}

int vector_append_constvector(vector_type *vec, constvector_type *cvec)
{
	int rc = 0, i;
	void *value;

	if(!cvec)
		return EINVAL;

	rc = __append_checks(vec, cvec->elements);
	if(rc)
		return rc;

	for(i = 0; i < cvec->elements; i++) {
		value = constvector_element(cvec, i);
		if(!value)
			return EFAULT;

		vector_set_value(vec, vec->elements, value);
		vec->elements++;
	}

	return 0;
}

int vector_contains(vector_type *vec, void * value)
{
	unsigned int i;

	if(!vec || !value || !vec->compare)
		return 0;

	for_all_vector_elements(vec, i) {
		if(vec->compare(vector_get_value(vec, i), value,
					vec->element_size) == 0) {
			return 1;
		}
	}

	return 0;
}

int vector_contains_sorted(vector_type *vec, void * value)
{
	int comp;
	unsigned int min, max, mid;
	char * current;

	if(!vec || !value || !vec->compare || !vec->elements)
		return 0;

	min = 0;
	max = vec->elements - 1;

	do {
		mid = (min + max) / 2;
		current = ((char *) vec->values) + (mid * vec->element_size);

		comp = vec->compare(value, current, vec->element_size);
		if(comp == 0)
			return 1;
		else if(comp < 0)
			max = mid - 1;
		else
			min = mid + 1;
	} while (min <= max);

	return 0;
}
