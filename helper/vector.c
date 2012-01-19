#include "vector.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include "list.h"
#include "constvector.h"
#include "algorithm.h"

#define CHARP(vec) ((char *) vec->values)
#define NELEMENTS(n, vec) ((n) * (vec)->element_size)

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

static int __append_checks(vector_type *vec, size_t src_size)
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

#define __append_template(dest, src, __get__) \
	int rc = 0, i; \
	void *value; \
	\
	if(!(src)) \
		return EINVAL; \
	\
	rc = __append_checks((dest), (src)->elements); \
	if(rc) \
		return rc; \
	\
	for(i = 0; i < (src)->elements; i++) { \
		value = __get__((src), i); \
		if(!value) \
			return EFAULT; \
		\
		vector_set_value((dest), (dest)->elements, value); \
		(dest)->elements++; \
	} \
	\
	return 0;


static int __pre_add_checks(vector_type *vec, void * value)
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

	return 0;
}

/************************************************
 *      public interface implementations        *
 ************************************************/

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

int vector_add_value(vector_type *vec, void * value)
{
	int rc = __pre_add_checks(vec, value);

	if(rc)
		return rc;

	vector_set_value(vec, vec->elements, value);
	vec->elements++;

	return 0;
}

int vector_insert(vector_type *vec, unsigned int i, void * value)
{
	int rc;

	if(!vec || (i > vec->elements))
		return EINVAL;

	rc = __pre_add_checks(vec, value);
	if(rc)
		return rc;

	memmove(CHARP(vec) + NELEMENTS(i + 1, vec),
			CHARP(vec) + NELEMENTS(i, vec),
			NELEMENTS(vec->elements - i, vec));

	vector_set_value(vec, i, value);
	vec->elements++;

	return 0;
}

static int __vector_insert_sorted(vector_type *vec, void * value,
		int allow_doubles, unsigned int *rpos)
{
	int rc;

	int pos, comp;
	void * current;
	unsigned int min, max, mid;

	rc = __pre_add_checks(vec, value);
	if(rc)
		return rc;

	if(!vec->compare)
		return EINVAL;

	if(vec->elements == 0) {
		if(rpos)
			*rpos = 0;
		return vector_add_value(vec, value);
	}

	min = 0;
	max = vec->elements - 1;

	do {
		mid = (min + max) / 2;
		current = CHARP(vec) + NELEMENTS(mid, vec);

		comp = vec->compare(current, value, vec->element_size);
		if(comp == 0) {
			if(!allow_doubles)
				return 0;
			break;
		} else if(comp > 0) {
			if((int) (mid - 1) < 0)
				break;
			max = mid - 1;
		}
		else
			min = mid + 1;
	} while (min <= max);

	if(comp < 0)
		pos = mid + 1;
	else
		pos = mid;

	rc = vector_insert(vec, pos, value);
	if(rc)
		return rc;

	if(rpos)
		*rpos = pos;
	return 0;
}

int vector_insert_sorted(vector_type *vec, void * value, int allow_doubles)
{
	return __vector_insert_sorted(vec, value, allow_doubles, NULL);
}

int vector_insert_sorted_pos(vector_type *vec, void * value, int allow_doubles,
		unsigned int *rpos)
{
	return __vector_insert_sorted(vec, value, allow_doubles, rpos);
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

int vector_append_list(vector_type *vec, list_type *list)
{
	__append_template(vec, list, list_element);
}

int vector_append_constvector(vector_type *vec, constvector_type *cvec)
{
	__append_template(vec, cvec, constvector_element);
}

int vector_append_vector(vector_type *dest, vector_type *src)
{
	__append_template(dest, src, vector_get_value);
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

int vector_is_sorted(vector_type *vec)
{
	void * max, *cur;
	unsigned int i;
	int comp;

	if(!vec || !vec->compare)
		return 0;

	if(vec->elements < 1)
		return 1;

	max = vector_get_value(vec, 0);

	for (i = 1; i < vec->elements; i++) {
		cur = vector_get_value(vec, i);
		comp = vec->compare(max, cur, vec->element_size);

		if(comp < 0)
			max = cur;
		else if(comp > 0)
			return 0;
	}

	return 1;
}

int vector_massmove(vector_type *vec, unsigned int from, unsigned int to,
		unsigned int newpos, vector_type *buf)
{
	int rc;
	char * run;
	size_t ssize;

	if(!vec || !vec->elements || !vec->values)
		return EINVAL;

	if(((to - from) < 0) || (from >= vec->elements) || (to >= vec->elements)
			|| (newpos > vec->elements))
		return EINVAL;

	if((newpos > from) && (newpos <= to))
		return EINVAL;

	if(from == newpos || (to + 1) == newpos)
		return 0;

	if(!buf || (buf->elements > 0))
		return EINVAL;

	rc = __append_checks(buf, vec->elements);
	if(rc)
		return rc;

	run = CHARP(buf);
	if(newpos == vec->elements) {
		/* move window to the end */

		memcpy(run, vec->values, NELEMENTS(from, vec));
		run += NELEMENTS(from, vec);

		memcpy(run, CHARP(vec) + NELEMENTS(to + 1, vec),
				NELEMENTS(vec->elements - to - 1, vec));
		run += NELEMENTS(vec->elements - to - 1, vec);

		memcpy(run, CHARP(vec) + NELEMENTS(from, vec),
				NELEMENTS(to - from + 1, vec));
	} else if(newpos == 0) {
		/* move window to the beginning */

		memcpy(run, CHARP(vec) + NELEMENTS(from, vec),
				NELEMENTS(to - from + 1, vec));
		run += NELEMENTS(to - from + 1, vec);

		memcpy(run, vec->values, NELEMENTS(from, vec));
		run += NELEMENTS(from, vec);

		memcpy(run, CHARP(vec) + NELEMENTS(to + 1, vec),
				NELEMENTS(vec->elements - to - 1, vec));
	} else if(newpos < from) {
		/* move before the first element of the window */

		memcpy(run, vec->values, NELEMENTS(newpos, vec));
		run += NELEMENTS(newpos, vec);

		memcpy(run, CHARP(vec) + NELEMENTS(from, vec),
				NELEMENTS(to - from + 1, vec));
		run += NELEMENTS(to - from + 1, vec);

		memcpy(run, CHARP(vec) + NELEMENTS(newpos, vec),
				NELEMENTS(from - newpos, vec));
		run += NELEMENTS(from - newpos, vec);

		memcpy(run, CHARP(vec) + NELEMENTS(to + 1, vec),
				NELEMENTS(vec->elements - to - 1, vec));
	} else {
		/* move after the last (+1) element of the window */

		memcpy(run, vec->values, NELEMENTS(from, vec));
		run += NELEMENTS(from, vec);

		memcpy(run, CHARP(vec) + NELEMENTS(to + 1, vec),
				NELEMENTS(newpos - to - 1, vec));
		run += NELEMENTS(newpos - to - 1, vec);

		memcpy(run, CHARP(vec) + NELEMENTS(from, vec),
				NELEMENTS(to - from + 1, vec));
		run += NELEMENTS(to - from + 1, vec);

		memcpy(run, CHARP(vec) + NELEMENTS(newpos, vec),
				NELEMENTS(vec->elements - newpos, vec));
	}

	run = vec->values;
	vec->values = buf->values;
	buf->values = run;

	ssize = vec->len;
	vec->len = buf->len;
	buf->len = ssize;

	return 0;
}

#undef __append_template
#undef CHARP
