#include "constvector.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include "vector.h"

struct memchunk {
	void * mem; /* memory provided by this chunk */
	size_t min_i; /* first element of the containing vector in this chunk */
	size_t len; /* size */
};

static unsigned int __log2base(unsigned int v)
{
	static const char LogTable256[256] = {
#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
		-1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
		LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
		LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
	};
	unsigned r;
	register unsigned int t, tt;

	if ((tt = v >> 16)) {
		r = (t = tt >> 8) ? 24 + LogTable256[t] : 16 + LogTable256[tt];
	} else {
		r = (t = v >> 8) ? 8 + LogTable256[t] : LogTable256[v];
	}

	return r;
}

static int __increase(constvector_type *vec, size_t inc)
{
	int rc = 0;
	struct memchunk chunk = { .mem = 0, .min_i = 0, .len = 0 };

	inc++;

	chunk.mem = calloc(inc, vec->element_size);
	if(!chunk.mem)
		return ENOMEM;

	/* TODO: test in the actual vector */
	chunk.min_i = inc - 1;
	chunk.len = inc;

	rc = vector_add_value(vec->memchunks, &chunk);
	if(rc)
		return rc;

	vec->len += inc;
	return 0;
}

static void * __get_element(constvector_type *vec, unsigned int i)
{
	unsigned int chunk = __log2base(i + 1);
	unsigned int chunk_pos = i + 1 - (1 << chunk);
	struct memchunk * memchunk;

	if(chunk >= vec->memchunks->elements)
		return NULL;

	memchunk = vector_get_value(vec->memchunks, chunk);

	if(!memchunk || (chunk_pos >= memchunk->len) || (i < memchunk->min_i))
		return NULL;

	return ((char *) memchunk->mem) + (chunk_pos * vec->element_size);
}

int constvector_create(constvector_type ** vec, size_t len, size_t element_size)
{
	int rc = 0;
	constvector_type * n_vec;

	if(!vec || !element_size)
		return EINVAL;

	n_vec = (constvector_type *) malloc(sizeof(*n_vec));
	if(!n_vec)
		return ENOMEM;

	memset(n_vec, 0, sizeof(*n_vec));

	rc = vector_create(&n_vec->memchunks, 0, sizeof(struct memchunk));
	if(rc)
		goto err_free;

	n_vec->len = 0;
	n_vec->elements = 0;
	n_vec->element_size = element_size;
	n_vec->compare = memcmp;

	*vec = n_vec;

	return 0;
err_free:
	free(n_vec);
	return rc;
}

void constvector_destroy(constvector_type * vec)
{
	int i;
	struct memchunk * ch;

	if(!vec)
		return;

	for_all_vector_elements(vec->memchunks, i) {
		ch = vector_get_value(vec->memchunks, i);
		if(ch && ch->mem)
			free(ch->mem);
	}
	vector_destroy(vec->memchunks);

	free(vec);
}

void * constvector_element(constvector_type *vec, unsigned int i)
{
	if(!vec || !vec->memchunks)
		return NULL;

	return __get_element(vec, i);
}

int constvector_get(constvector_type *vec, unsigned int i, void * value)
{
	void * mempos;

	if(!vec || !vec->memchunks || !value)
		return EINVAL;

	mempos = __get_element(vec, i);
	if(!mempos)
		return EINVAL;

	memcpy(value, mempos, vec->element_size);
	return 0;
}

void constvector_set(constvector_type *vec, unsigned int i, void * value)
{
	void * mempos;

	if(!vec || !vec->memchunks || !value)
		return;

	mempos = __get_element(vec, i);
	if(!mempos)
		return;

	memcpy(mempos, value, vec->element_size);
}

int constvector_add(constvector_type *vec, void * value)
{
	int rc = 0;
	void * mempos;

	if(!vec || !vec->memchunks || !value)
		return EINVAL;

	if(vec->elements >= vec->len) {
		rc = __increase(vec, vec->len);
		if(rc)
			return rc;
	}

	mempos = __get_element(vec, vec->elements);
	if(!mempos)
		return EINVAL;

	memcpy(mempos, value, vec->element_size);
	vec->elements += 1;
	return 0;
}

int constvector_contains(constvector_type *vec, void * value)
{
	int i;

	if(!vec || !value)
		return 0;

	for(i = 0; i < vec->elements; i++)
		if(vec->compare(constvector_element(vec, i), value,
					vec->element_size) == 0)
			return 1;

	return 0;
}

int constvector_del(constvector_type *vec, unsigned int i)
{
	return EPERM;
}
