#include "matrixgraph.h"

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>

#include "matrix.h"

struct node {
	unsigned char neighbors;
};

#define NODE_POS_MASK		0x1
#define NODE_ACCESS_MEM(node)	(&(node)[1])
#define NODE_WRITE_MEM(node, val, size) \
	memcpy(NODE_ACCESS_MEM((node)), (val), (size))

#define GRAPH_ACCESS_NODE(graph, i, j) \
	((struct node *) matrix_get((graph)->matrix, (i), (j)))

#define GRAPH_NEIGHBORS_TOP_RIGHT	0xD0
#define GRAPH_NEIGHBORS_TOP_LEFT	0x68
#define GRAPH_NEIGHBORS_BOTTOM_RIGHT	0x16
#define GRAPH_NEIGHBORS_BOTTOM_LEFT	0xB

#define GRAPH_NEIGHBORS_CENTER		0xFF

#define GRAPH_NEIGHBORS_TOP_ROW		0xF8
#define GRAPH_NEIGHBORS_LEFT_ROW	0xD6
#define GRAPH_NEIGHBORS_RIGHT_ROW	0x6B
#define GRAPH_NEIGHBORS_BOTTOM_ROW	0x1F

static inline long
has_node_neighbor(struct node *node, enum node_neighbor_position pos)
{
	return ((node->neighbors >> pos) & NODE_POS_MASK);
}

static inline void *
get_node_mem(matrix_graph_type *matg, size_t i, size_t j)
{
	struct node *node = GRAPH_ACCESS_NODE(matg, i, j);
	void * mem = NODE_ACCESS_MEM(node);
	return mem;
}

static inline void
add_node_neighbor(struct node *node, enum node_neighbor_position pos)
{
	node->neighbors |= (((unsigned char) 0x1) << pos);
}

static inline void
del_node_neighbor(struct node *node, enum node_neighbor_position pos)
{
	node->neighbors &= ~(((unsigned char) 0x1) << pos);
}

static inline void
set_node_neighbor(struct node *node, enum node_neighbor_position pos, long val)
{
	if(val)
		 add_node_neighbor(node, pos);
	else
		 del_node_neighbor(node, pos);
}

static inline int
get_neighbor_pos(size_t *i, size_t *j, enum node_neighbor_position pos)
{
	switch(pos)  {
	case NODE_TOP_LEFT:		(*i)--; (*j)--;	break;
	case NODE_TOP_CENTER:		(*i)--;		break;
	case NODE_TOP_RIGHT:		(*i)--; (*j)++;	break;
	case NODE_MIDDLE_LEFT:		(*j)--;		break;
	case NODE_MIDDLE_RIGHT:		(*j)++;		break;
	case NODE_BOTTOM_LEFT:		(*i)++; (*j)--;	break;
	case NODE_BOTTOM_CENTER:	(*i)++;		break;
	case NODE_BOTTOM_RIGHT:		(*i)++; (*j)++;	break;
	default:			return EINVAL;
	}

	return 0;
}

/**********************************************************/
/*                                                        */
/* below are the implementations for the public interface */
/*                                                        */
/**********************************************************/

int
matrix_graph_create(matrix_graph_type **matg, size_t m, size_t n,
		size_t node_size)
{
	int rc = 0;
	matrix_graph_type *new_matg;

	if(!matg || !node_size || !m || !n)
		return EINVAL;

	new_matg = (matrix_graph_type *) malloc(sizeof(*new_matg));
	if(!new_matg)
		return ENOMEM;

	rc = matrix_create(&new_matg->matrix, m, n,
			node_size+sizeof(struct node));
	if(rc)
		goto err_free;

	*matg = new_matg;

	return rc;
err_free:
	free(new_matg);
	return rc;
}

void
matrix_graph_destroy(matrix_graph_type *matg)
{
	if(!matg)
		return;

	matrix_destroy(matg->matrix);
	free(matg);
}

int
matrix_graph_init(matrix_graph_type *matg, void * node_value)
{
	int i, j;
	struct node *node;
	size_t size = matg->matrix->element_size - sizeof(*node);

	matrix_type *mat;

	if(!matg || !node_value)
		return EINVAL;

	mat = matg->matrix;

	if(!mat)
		return EFAULT;

#define INIT_NODE(i, j, n) {\
	node = GRAPH_ACCESS_NODE(matg, (i), (j)); \
	node->neighbors = (n); \
	NODE_WRITE_MEM(node, node_value, size); \
}

	/* top right corner */
	INIT_NODE(0, 0, GRAPH_NEIGHBORS_TOP_RIGHT);

	/* bottem right corner */
	INIT_NODE(mat->m - 1, 0, GRAPH_NEIGHBORS_BOTTOM_RIGHT);

	/* top left corner */
	INIT_NODE(0, mat->n - 1, GRAPH_NEIGHBORS_TOP_LEFT);

	/* bottom left corner */
	INIT_NODE(mat->m - 1, mat->n - 1, GRAPH_NEIGHBORS_BOTTOM_LEFT);

	/* top row */
	for(i = 0, j = 1; j < mat->n - 1; j++)
		INIT_NODE(i, j, GRAPH_NEIGHBORS_TOP_ROW);

	/* left row */
	for(i = 1, j = 0; i < mat->m - 1; i++)
		INIT_NODE(i, j, GRAPH_NEIGHBORS_LEFT_ROW);

	/* right row */
	for(i = 1, j = mat->n - 1; i < mat->m - 1; i++)
		INIT_NODE(i, j, GRAPH_NEIGHBORS_RIGHT_ROW);

	/* top row */
	for(i = mat->m - 1, j = 1; j < mat->n - 1; j++)
		INIT_NODE(i, j, GRAPH_NEIGHBORS_BOTTOM_ROW);

	for(i = 1; i < mat->m - 1; i++)
		for(j = 1; j < mat->n - 1; j++)
			INIT_NODE(i, j, GRAPH_NEIGHBORS_CENTER);

#undef INIT_NODE

	return 0;
}

void *
matrix_graph_get_node(matrix_graph_type *matg, size_t i, size_t j)
{
	if(!matg || !matrix_index_valid(matg->matrix, i, j))
		return NULL;
	return get_node_mem(matg, i, j);
}

void *
matrix_graph_get_neighbor(matrix_graph_type *matg, size_t i, size_t j,
		enum node_neighbor_position pos)
{
	if(!matg)
		return NULL;

	/* changes i and j into the neighbors coordinates */
	if(get_neighbor_pos(&i, &j, pos))
		return NULL;

	if(!matrix_index_valid(matg->matrix, i, j))
		return NULL;

	return get_node_mem(matg, i, j);
}

void
matrix_graph_del_neighbor(matrix_graph_type *matg, size_t i, size_t j,
		enum node_neighbor_position pos)
{
	if(!matg || !matrix_index_valid(matg->matrix, i, j))
		return;

	del_node_neighbor(GRAPH_ACCESS_NODE(matg, i, j), pos);
}

void
matrix_graph_add_neighbor(matrix_graph_type *matg, size_t i, size_t j,
		enum node_neighbor_position pos)
{
	size_t n_i = i, n_j = j;

	if(!matg || !matrix_index_valid(matg->matrix, i, j))
		return;

	if(get_neighbor_pos(&n_i, &n_j, pos))
		return;

	if(!matrix_index_valid(matg->matrix, i, j))
		return;

	add_node_neighbor(GRAPH_ACCESS_NODE(matg, i, j), pos);
}

char *
matrix_graph_strneighbors(matrix_graph_type *matg, size_t i, size_t j)
{
	static char str[8];
	struct node *node;
	int k;
	long has;

	memset(str, 0, sizeof(str));

	if(!matg || !matrix_index_valid(matg->matrix, i, j))
		return str;

	node = GRAPH_ACCESS_NODE(matg, i, j);
	for(k = NODE_FIRST_NEIGHBOR; k < NODE_MAX_NEIGHBORS; k++) {
		has = has_node_neighbor(node, k);
		if(!has) {
			str[k] = ' ';
			continue;
		}

		switch (k) {
		case NODE_TOP_LEFT:
			str[k] = '\\';
			break;
		case NODE_TOP_CENTER:
			str[k] = '^';
			break;
		case NODE_TOP_RIGHT:
			str[k] = '/';
			break;
		case NODE_MIDDLE_LEFT:
			str[k] = '<';
			break;
		case NODE_MIDDLE_RIGHT:
			str[k] = '>';
			break;
		case NODE_BOTTOM_LEFT:
			str[k] = '/';
			break;
		case NODE_BOTTOM_CENTER:
			str[k] = '_';
			break;
		case NODE_BOTTOM_RIGHT:
			str[k] = '\\';
			break;
		}
	}

	return str;
}

enum node_neighbor_position
matrix_graph_neighbor_opposite(enum node_neighbor_position pos)
{
	switch(pos)  {
	case NODE_TOP_LEFT:		return NODE_BOTTOM_RIGHT;
	case NODE_TOP_CENTER:		return NODE_BOTTOM_CENTER;
	case NODE_TOP_RIGHT:		return NODE_BOTTOM_LEFT;
	case NODE_MIDDLE_LEFT:		return NODE_MIDDLE_RIGHT;
	case NODE_MIDDLE_RIGHT:		return NODE_MIDDLE_LEFT;
	case NODE_BOTTOM_LEFT:		return NODE_TOP_RIGHT;
	case NODE_BOTTOM_CENTER:	return NODE_TOP_CENTER;
	case NODE_BOTTOM_RIGHT:		return NODE_TOP_LEFT;
	default:			return NODE_MAX_NEIGHBORS;
	}
}

/**********************************************************/
/*                                                        */
/* below is the implementation of the neighbor-iterator   */
/*                                                        */
/**********************************************************/

struct matrix_graph_neighbor_iterator {
	matrix_graph_type *matg;

	unsigned char neighbors;
	unsigned char pos;
	size_t i, j;
};

int
matrix_graph_neighbor_iterator_create(
		struct matrix_graph_neighbor_iterator **it,
		matrix_graph_type *matg, size_t i, size_t j)
{
	struct matrix_graph_neighbor_iterator *new_it;

	if(!it || !matg || !matrix_index_valid(matg->matrix, i, j))
		return EINVAL;

	new_it = (struct matrix_graph_neighbor_iterator *)
			malloc(sizeof(*new_it));
	if(!new_it)
		return ENOMEM;

	new_it->matg = matg;
	new_it->neighbors = GRAPH_ACCESS_NODE(matg, i, j)->neighbors;
	new_it->pos = 0;
	new_it->i = i;
	new_it->j = j;

	*it = new_it;

	return 0;
}

void
matrix_graph_neighbor_iterator_destroy(
		struct matrix_graph_neighbor_iterator *it)
{
	if(!it)
		return;

	free(it);
}

int
matrix_graph_neighbor_iterator_reinit(
		struct matrix_graph_neighbor_iterator *it,
		matrix_graph_type *matg, size_t i, size_t j)
{
	if(!it || !matg || !matrix_index_valid(matg->matrix, i, j))
		return EINVAL;

	it->matg = matg;
	it->neighbors = GRAPH_ACCESS_NODE(matg, i, j)->neighbors;
	it->pos = 0;
	it->i = i;
	it->j = j;

	return 0;
}

long
matrix_graph_neighbor_iterator_has_next(
		struct matrix_graph_neighbor_iterator *it)
{
	return (it && (it->neighbors != 0));
}

int
matrix_graph_neighbor_iterator_next(
		struct matrix_graph_neighbor_iterator *it,
		matrix_graph_node_type *node)
{
	unsigned char n;
	unsigned char pos;
	size_t i, j;

	if(!it || !it->neighbors || !node)
		return EINVAL;

	n = it->neighbors;
	pos = it->pos;
	i = it->i;
	j = it->j;

	while (!(n & NODE_POS_MASK)) {
		n >>= 1;
		pos++;
	}

	if(get_neighbor_pos(&i, &j, pos))
		return EFAULT;

	if(!matrix_index_valid(it->matg->matrix, i, j))
		return EFAULT;

	it->neighbors = n >> 1;
	it->pos = pos + 1;

	node->mem = get_node_mem(it->matg, i, j);
	node->rel_pos = pos;
	node->i = i;
	node->j = j;
	return 0;
}
