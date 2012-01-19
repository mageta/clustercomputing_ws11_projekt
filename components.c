#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>

#include "matrix.h"
#include "queue.h"
#include "stack.h"
#include "vector.h"
#include "constvector.h"
#include "list.h"
#include "components.h"
#include "matrixgraph.h"

#define err_if(rc) if(rc && fprintf(stderr, "ERR in %s:%d:%s(): %s\n", __FILE__, __LINE__, __func__, strerror(rc)))

/* we distinguish this to save memory in the graph */
struct node {
	int i, j;
};

struct graph_node {
	unsigned char color;
	struct component *component;
} __attribute__((__packed__));

static int
borders_create(vector_type **borders, matrix_type *mat)
{
	int rc = 0, i;
	vector_type *bord;
	matrix_type *border;

	if(!borders || !mat)
		return EINVAL;

	rc = vector_create(&bord, BORDER_MAX, sizeof(*border));
	err_if(rc)
		return ENOMEM;

	for(i = BORDER_MIN; i < BORDER_MAX; i++) {
		switch(i) {
		case BORDER_LEFT:
		case BORDER_RIGHT:
			rc = matrix_create(&border, mat->m, 1,
					sizeof(unsigned int));
			break;
		case BORDER_TOP:
		case BORDER_BOTTOM:
			rc = matrix_create(&border, 1, mat->n,
					sizeof(unsigned int));
			break;
		}

		err_if(rc)
			goto err_out;

		vector_set_value(bord, i, border);

		/*
		 * just free the containing struct, these informations are in
		 * the vector
		 */
		free(border);
	}

	/* because we didn't use add */
	bord->elements = BORDER_MAX;

	*borders = bord;
	return 0;
err_out:
	vector_destroy(bord);
	return rc;
}

void
borders_destroy(vector_type *borders)
{
	int i, max;
	matrix_type *border;

	max = borders->elements;
	for(i = 0; i < max; i++) {
		border = vector_get_value(borders, 0);
		if(!border || !border->matrix)
			continue;

		free(border->matrix);
	}

	vector_destroy(borders);
}

static int
borders_fill(vector_type *borders, matrix_graph_type *graph)
{
	int i, j, k;
	struct graph_node *node;
	matrix_type *border;

	if(!borders || !graph || !graph->matrix)
		return EINVAL;

	for(j = 0, k = 0; j < graph->matrix->n && k < 2;
			j = (j + graph->matrix->n - 1), k++) {
		if(k == 0)
			border = vector_get_value(borders, BORDER_LEFT);
		else
			border = vector_get_value(borders, BORDER_RIGHT);

		if(!border || (border->m != graph->matrix->m))
			return EFAULT;

		for(i = 0; i < border->m; i++) {
			node = (struct graph_node *)
				matrix_graph_get_node(graph, i, j);

			if(!node)
				return EFAULT;
			if(!node->component)
				continue;

			matrix_set(border, i, 0,
					&node->component->component_id);
		}
	}

	for(i = 0, k = 0; i < graph->matrix->m && k < 2;
			i = (i + graph->matrix->m - 1), k++) {
		if(k == 0)
			border = vector_get_value(borders, BORDER_TOP);
		else
			border = vector_get_value(borders, BORDER_BOTTOM);

		if(!border || (border->n != graph->matrix->n))
			return EFAULT;

		for(j = 0; j < border->n; j++) {
			node = (struct graph_node *)
				matrix_graph_get_node(graph, i, j);

			if(!node)
				return EFAULT;
			if(!node->component)
				continue;

			matrix_set(border, 0, j,
					&node->component->component_id);
		}
	}

	return 0;
}

static int
visit_node(stack_type *to_be_visited,
		struct graph_node *node, struct node *coords)
{
	int rc = 0;

	/*
	 * push unvisited nodes onto the stack
	 * white nodes are unvisited,
	 * gray nodes already on the stack,
	 * black nodes are already expanded
	 */
	if(node->color < 1) { /* white */
		rc = stack_push(to_be_visited, coords);
		err_if(rc)
			goto err_enqueue;

		node->color = 1; /* gray */
	}

	return 0;
err_enqueue:
	return rc;
}

static int
complete_component(matrix_type *mat,
		matrix_graph_type *graph,
		stack_type *to_be_visited,
		constvector_type *components,
		struct graph_node *search_node,
		struct node *search_coords)
{
	static stack_type *black_neighbours = 0;
	static struct matrix_graph_neighbor_iterator *neighbours;

	int rc = 0;
	struct node coords, current_coords;
	unsigned char value;

	struct graph_node *node;
	struct component comp;

	matrix_graph_node_type iter_node;

	if(!mat || !graph || !to_be_visited || !components ||
			!search_node || !search_node)
		return EINVAL;

	value = *((unsigned char *) matrix_get(mat, search_coords->i,
				search_coords->j));
	/*
	 * if the value is not black or if this component was
	 * already completed
	 */
	if(!value || search_node->component)
		return 0;

	if(!black_neighbours) {
		/*
		 * somewhat hacky, but in cases where this function is often
		 * called, we safe the time to reallocate memory
		 */
		rc = stack_create(&black_neighbours, sizeof(*search_coords));
		err_if(rc)
			goto err_stack_bn;
	}

	if(!neighbours) {
		/* same reason as above */
		rc = matrix_graph_neighbor_iterator_create(&neighbours,
				graph, search_coords->i, search_coords->j);
		err_if(rc)
			goto err_iter_neighbours;
	}

	/* init a new component for the search_node */
	memset(&comp, 0, sizeof(comp));
	comp.example_coords[0] = search_coords->i;
	comp.example_coords[1] = search_coords->j;
	comp.component_id = components->elements + 1;
	comp.size = 1;

	rc = constvector_add(components, &comp);
	err_if(rc)
		goto err_component_create;

	search_node->component = (struct component *) constvector_element(
			components, components->elements - 1);

	/* now add all connected black nodes to this component */
	rc = stack_push(black_neighbours, search_coords);
	err_if(rc)
		goto err_push_bn;

	while(stack_size(black_neighbours)) {
		stack_pop(black_neighbours, &current_coords);

		rc = matrix_graph_neighbor_iterator_reinit(neighbours,
				graph, current_coords.i, current_coords.j);
		err_if(rc)
			goto err_iterator_reinit;

		while(matrix_graph_neighbor_iterator_has_next(neighbours)) {
			matrix_graph_neighbor_iterator_next(neighbours,
					&iter_node);

			node = (struct graph_node *) iter_node.mem;
			coords.i = iter_node.i;
			coords.j = iter_node.j;

			value = *((unsigned char *) matrix_get(mat,
					coords.i, coords.j));

			if(!node->component && value) {
				node->component = search_node->component;
				node->component->size += 1;

				rc = stack_push(black_neighbours,
						&coords);
				err_if(rc)
					goto err_push_bn;

				node->color = 2;
			}
			else {
				rc = visit_node(to_be_visited, node, &coords);
				err_if(rc)
					goto err_visit_node;
			}

			/*
			 * there is no need to look at this edge again
			 */
			matrix_graph_del_neighbor(graph,
					current_coords.i, current_coords.j,
					iter_node.rel_pos);
			matrix_graph_del_neighbor(graph,
					coords.i, coords.j,
					matrix_graph_neighbor_opposite(
						iter_node.rel_pos));
		}
	}

	/*
	 * as long as the stack is static
	 *
	 * stack_destroy(black_neighbours);
	 */

	return 0;
err_visit_node:
err_iterator_reinit:
err_push_bn:
err_component_create:
	matrix_graph_neighbor_iterator_destroy(neighbours);
err_iter_neighbours:
	stack_destroy(black_neighbours);
err_stack_bn:
	return rc;
}

int
find_components(matrix_type *mat,
		struct component_list **comp_list,
		vector_type **borders)
{
	int rc = 0;

	stack_type *to_be_visited = 0;
	constvector_type *tmp_comp;
	vector_type *bord = NULL;
	matrix_graph_type *graph;
	matrix_graph_node_type iter_node;

	struct component comp;
	struct component_list *compl_p;
	struct node coords, current_coords;
	struct graph_node init_node, *node, *current_node;
	struct matrix_graph_neighbor_iterator *neighbours;

	if(!comp_list)
		return EINVAL;

	rc = component_list_create(&compl_p);
	err_if(rc)
		goto err_compl_create;

	if(borders) {
		rc = borders_create(&bord, mat);
		err_if(rc)
			goto err_borders_create;
	}

	rc = constvector_create(&tmp_comp, 0, sizeof(comp));
	err_if(rc)
		goto err_list_tc;

	rc = matrix_graph_create(&graph, mat->m, mat->n, sizeof(*node));
	err_if(rc)
		goto err_graph_create;

	/*
	 * 0 - white
	 * 1 - gray
	 * 2 - black
	 */
	init_node.color = 0;
	init_node.component = NULL;
	rc = matrix_graph_init(graph, &init_node);
	err_if(rc)
		goto err_graph_init;

	rc = matrix_graph_neighbor_iterator_create(&neighbours,
			graph, 0, 0);
	err_if(rc)
		goto err_iter_neighbours;

	rc = stack_create(&to_be_visited, sizeof(coords));
	err_if(rc)
		goto err_queue_tbv;

	node = (struct graph_node *) matrix_graph_get_node(graph, 0, 0);
	coords.i = 0;
	coords.j = 0;

	/* initial coordinates to expand */
	rc = stack_push(to_be_visited, &coords);
	err_if(rc)
		goto err_enqueue_tbv;

	/* look at the initial node */
	rc = complete_component(mat, graph, to_be_visited,
			tmp_comp, node, &coords);
	err_if(rc)
		goto err_complete_comp;

	while(stack_size(to_be_visited)) {
		stack_pop(to_be_visited, &current_coords);

		current_node = (struct graph_node *)
					matrix_graph_get_node(graph,
						current_coords.i, current_coords.j);

		if(current_node->color > 1)
			continue;
		/* color it black */
		current_node->color = 2;

		/* reinit the iterator with the new node */
		rc = matrix_graph_neighbor_iterator_reinit(neighbours,
				graph, current_coords.i, current_coords.j);
		err_if(rc)
			goto err_iterator_reinit;

		/* look at all neighbours */
		while(matrix_graph_neighbor_iterator_has_next(neighbours)) {
			matrix_graph_neighbor_iterator_next(neighbours,
					&iter_node);

			node = (struct graph_node *) iter_node.mem;
			/* use the coordinates of this neighbour */
			coords.i = iter_node.i;
			coords.j = iter_node.j;

			rc = complete_component(mat, graph, to_be_visited,
					tmp_comp, node, &coords);
			err_if(rc)
				goto err_complete_comp;

			rc = visit_node(to_be_visited, node, &coords);
			err_if(rc)
				goto err_visit_node;

			/*
			 * there is no need to look at this edge again
			 */
			matrix_graph_del_neighbor(graph,
					current_coords.i, current_coords.j,
					iter_node.rel_pos);
			matrix_graph_del_neighbor(graph,
					coords.i, coords.j,
					matrix_graph_neighbor_opposite(
						iter_node.rel_pos));
		}
	}
{ /* only for debugging, can be removed later */
	int i, j;
	fprintf(stdout, "\n");
	for(i = 0; i < graph->matrix->m; i++) {
		for(j = 0; j < (graph->matrix->n - 1); j++) {
			node = (struct graph_node *)	matrix_graph_get_node(graph, i, j);

			if(node->component)
				fprintf(stdout, "%2u, ", node->component->component_id);
			else
				fprintf(stdout, "  , ");
		}

		node = (struct graph_node *)	matrix_graph_get_node(graph, i, j);

		if(node->component)
			fprintf(stdout, "%2u\n", node->component->component_id);
		else
			fprintf(stdout, "  \n");
	}
	fprintf(stdout, "\n");
}

	if(bord) {
		rc = borders_fill(bord, graph);
		err_if(rc)
			goto err_fill_borders;

		*borders = bord;
	}

	stack_destroy(to_be_visited);
	matrix_graph_destroy(graph);

	rc = vector_append_constvector(compl_p->components, tmp_comp);
	err_if(rc)
		goto err_vecappend_components;
	*comp_list = compl_p;

	constvector_destroy(tmp_comp);

	return 0;
err_fill_borders:
err_iterator_reinit:
err_visit_node:
err_complete_comp:
err_enqueue_tbv:
	stack_destroy(to_be_visited);
err_queue_tbv:
	matrix_graph_neighbor_iterator_destroy(neighbours);
err_iter_neighbours:
err_graph_init:
	matrix_graph_destroy(graph);
err_vecappend_components:
err_graph_create:
	constvector_destroy(tmp_comp);
err_list_tc:
	if(bord) borders_destroy(bord);
err_borders_create:
	component_list_destroy(compl_p);
err_compl_create:
	return rc;
}

int
component_create(struct component ** comp)
{
	struct component *c;

	if(!comp)
		return EINVAL;

	c = (struct component *) malloc(sizeof(*c));
	if(!c)
		return ENOMEM;

	memset(c, 0, sizeof(*c));

	*comp = c;
	return 0;
}

void
component_destroy(struct component * comp)
{
	if(!comp)
		return;

	free(comp);
}

int component_compare(const void *lhv, const void *rhv, size_t size)
{
	struct component *lhc = (struct component *) lhv;
	struct component *rhc = (struct component *) rhv;

	if(lhc->component_id < rhc->component_id)
		return -1;
	else if(lhc->component_id > rhc->component_id)
		return 1;
	else
		return 0;
}

int
component_list_create(struct component_list ** compl)
{
	int rc = 0;
	struct component_list *cl;

	if(!compl)
		return EINVAL;

	cl = (struct component_list *) malloc(sizeof(*cl));
	if(!cl)
		return ENOMEM;

	memset(cl, 0, sizeof(*cl));

	rc = vector_create(&cl->components, 0, sizeof(struct component));
	err_if(rc)
		goto err_vector_create;

	cl->components->compare = component_compare;

	*compl = cl;

	return 0;
err_vector_create:
	free(cl);
	return rc;
}

void
component_list_destroy(struct component_list * compl)
{
	if(!compl)
		return;

	vector_destroy(compl->components);
	free(compl);
}

char * strborder(enum border_nr nr)
{
	static char str[10];

	switch(nr) {
		case BORDER_LEFT:
			strcpy(str, "LEFT"); break;
		case BORDER_TOP:
			strcpy(str, "TOP"); break;
		case BORDER_RIGHT:
			strcpy(str, "RIGHT"); break;
		case BORDER_BOTTOM:
			strcpy(str, "BOTTOM"); break;
		default:
			strcpy(str, "UNDEFINED"); break;
	}

	return str;
}
