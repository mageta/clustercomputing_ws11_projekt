#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <stdio.h>

#include "list.h"
#include "vector.h"
#include "stack.h"
#include "queue.h"

#define min(x, y) ({ \
	__typeof__(x) _min1 = (x);		\
	__typeof__(y) _min2 = (y);		\
	(void) (&_min1 == &_min2);	\
	_min1 < _min2 ? _min1 : _min2;	\
})

#define min_const(x, y) ({ \
	__typeof__(x) _min1 = (x);	\
	_min1 < (y) ? _min1 : (y);	\
})

void * bsearch_vector(vector_type *vec, void * key, unsigned int *pos);
void * bsearch_list(list_type *list, void * key, unsigned int *pos);

int merge_sorted_vector(vector_type *dest, vector_type *src);

#endif // ALGORITHM_H
