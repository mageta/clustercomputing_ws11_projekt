#ifndef MATRIXGRAPH_H
#define MATRIXGRAPH_H

#include <stdio.h>
#include <string.h>

#include "matrix.h"

struct matrix_graph {
	matrix_type * matrix;
};

enum node_neighbor_position {
	NODE_FIRST_NEIGHBOR	= 0x0,
	NODE_TOP_LEFT		= 0x0,
	NODE_TOP_CENTER		= 0x1,
	NODE_TOP_RIGHT		= 0x2,
	NODE_MIDDLE_LEFT	= 0x3,
	NODE_MIDDLE_RIGHT	= 0x4,
	NODE_BOTTOM_LEFT	= 0x5,
	NODE_BOTTOM_CENTER	= 0x6,
	NODE_BOTTOM_RIGHT	= 0x7,
	NODE_MAX_NEIGHBORS
};

struct matrix_graph_node {
	void * mem;
	size_t i, j;
	enum node_neighbor_position rel_pos;
};

typedef struct matrix_graph matrix_graph_type;
typedef struct matrix_graph_node matrix_graph_node_type;

int matrix_graph_create(matrix_graph_type **matg, size_t m, size_t n,
		size_t node_size) __attribute__((warn_unused_result));
void matrix_graph_destroy(matrix_graph_type *matg);
int matrix_graph_init(matrix_graph_type *matg, void * node_value);

void * matrix_graph_get_node(matrix_graph_type *matg, size_t i, size_t j)
	__attribute__((warn_unused_result));

void * matrix_graph_get_neighbor(matrix_graph_type *matg, size_t i, size_t j,
		enum node_neighbor_position pos)
		__attribute__((warn_unused_result));
void matrix_graph_del_neighbor(matrix_graph_type *matg, size_t i, size_t j,
		enum node_neighbor_position pos);
void matrix_graph_add_neighbor(matrix_graph_type *matg, size_t i, size_t j,
		enum node_neighbor_position pos);

char * matrix_graph_strneighbors(matrix_graph_type *matg, size_t i, size_t j)
	__attribute__((warn_unused_result));

enum node_neighbor_position
	matrix_graph_neighbor_opposite(enum node_neighbor_position pos);

/* neighbor-iterator.. some sort of :D */

struct matrix_graph_neighbor_iterator;

int matrix_graph_neighbor_iterator_create(
		struct matrix_graph_neighbor_iterator **it,
		matrix_graph_type *matg, size_t i, size_t j)
		__attribute__((warn_unused_result));
void matrix_graph_neighbor_iterator_destroy(
		struct matrix_graph_neighbor_iterator *it);
int matrix_graph_neighbor_iterator_reinit(
		struct matrix_graph_neighbor_iterator *it,
		matrix_graph_type *matg, size_t i, size_t j)
		__attribute__((warn_unused_result));

/* these functions do less runtime-checking, be aware! */

long matrix_graph_neighbor_iterator_has_next(
		struct matrix_graph_neighbor_iterator *it)
		__attribute__((warn_unused_result));
int matrix_graph_neighbor_iterator_next(
		struct matrix_graph_neighbor_iterator *it,
		matrix_graph_node_type *node);

#endif // MATRIXGRAPH_H
