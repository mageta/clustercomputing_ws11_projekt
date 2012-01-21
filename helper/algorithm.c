#include "algorithm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include "list.h"
#include "vector.h"
#include "stack.h"
#include "queue.h"

#define __bsearch_pos_pattern(dest, key, from, to, comp, pos, current, __current_get__) {\
	unsigned int min = (from); \
	unsigned int max = (to); \
	unsigned int mid; \
	\
	do { \
		mid = (min + max) / 2; \
		__current_get__; \
		\
		(comp) = (dest)->compare((current), (key), (dest)->element_size); \
		if((comp) == 0) { \
			(pos) = mid; \
			break; \
		} \
		else if((comp) > 0) { \
			if((int) (mid - 1) < 0) \
				break; \
			max = mid - 1; \
		} \
		else \
			min = mid + 1; \
	} while (min <= max); \
	(pos) = mid; \
}

#define __bsearch_pattern(dest, key, from, to, rpos, __current_get__) \
	int comp; \
	unsigned int pos; \
	char * current; \
	\
	if(!(dest) || !(key) || !(dest)->compare || !(dest)->elements) \
		return NULL; \
	\
	__bsearch_pos_pattern(dest, key, from, to, \
			comp, pos, current, __current_get__); \
	\
	if(comp == 0) {\
		if((rpos)) \
			*(rpos) = pos; \
		return current; \
	} \
	return NULL;

void * bsearch_vector(vector_type *vec, void * key, unsigned int *rpos)
{
	__bsearch_pattern(vec, key, 0, vec->elements - 1, rpos,
			current = ((char *) vec->values) +
				(mid * vec->element_size)
			);
}

unsigned int bsearch_vector_sortedpos(vector_type *vec, void * key,
		unsigned int from, unsigned int to)
{
	int comp;
	unsigned int pos;
	char * current;

	__bsearch_pos_pattern(vec, key, from, to,
			comp, pos, current,
			current = ((char *) vec->values) +
				(mid * vec->element_size));

	if(comp < 0)
		pos += 1;

	if(pos <= to) {
		current = ((char *) vec->values) + (pos * vec->element_size);
		comp = vec->compare(current, key, vec->element_size);

		while(comp < 0) {
			current += vec->element_size;
			if((++pos) > to)
				break;

			comp = vec->compare(current, key, vec->element_size);
		}
	}

	if(pos > from) {
		current = ((char *) vec->values) + (pos  * vec->element_size);
		comp = vec->compare(current - vec->element_size, key,
				vec->element_size);

		while(comp > 0) {
			current -= vec->element_size;
			if((--pos) == from)
				break;

			comp = vec->compare(current - vec->element_size, key,
					vec->element_size);
		}
	}

	return pos;
}

void * bsearch_list(list_type *list, void * key, unsigned int *rpos)
{
	__bsearch_pattern(list, key, 0, list->elements - 1, rpos,
			/* TODO: this is very inefficient */
			current = (char *) list_element(list, mid)
			);
}

int merge_sorted_vector(vector_type *dest, vector_type *src)
{
	int rc = 0, i, added;
	unsigned int lhpos, rhpos;
	size_t es;
	void *lhval, *rhval;
	int (* compare)(const void *, const void *, size_t);

	if((!dest || !dest->compare || !dest->element_size) ||
		(!src || !src->compare || !src->element_size) ||
		(src->compare != dest->compare) ||
		(src->element_size != dest->element_size))
		return EINVAL;

	compare = src->compare;
	es = src->element_size;

	if(dest->elements == 0)
		return vector_append_vector(dest, src);
	if(src->elements == 0)
		return 0;

	for(i = 0, lhpos = 0, rhpos = 0, added = 0;
			i < src->elements; i++, rhpos++, added = 0) {
		rhval = vector_get_value(src, rhpos);

		if(lhpos >= dest->elements) {
			rc = vector_add_value(dest, rhval);
			if(rc) return rc;
			continue;
		}
		lhval = vector_get_value(dest, lhpos);

		while(compare(lhval, rhval, es) <= 0) {
			lhpos++;

			if(lhpos >= dest->elements) {
				rc = vector_add_value(dest, rhval);
				if(rc) return rc;
				added = 1;
				break;
			}
			lhval = vector_get_value(dest, lhpos);
		}

		if(added)
			continue;

		rc = vector_insert(dest, lhpos, rhval);
		/*
		 * this is bad, because it could be, that we already
		 * changed the dest-vector :-/
		 */
		if(rc)
			return rc;
	}

	return 0;
}
