#ifndef COMPONENTS_H
#define COMPONENTS_H

#define COMM_DIMS 2

#include <stdio.h>

#include "vector.h"

struct component {
	int proc_rank;
	/* [0] = i; [1] = j; matrix-dimension-related */
	unsigned int example_coords[COMM_DIMS];
	unsigned int size;
	unsigned int component_id;
} __attribute__((__packed__));

/* packed because this shall be transmitted via mpi */

int component_create(struct component ** comp);

struct component_list {
	/* containes 'struct component' */
	vector_type *components;
};

int find_components(matrix_type *mat);

#endif // COMPONENTS_H
