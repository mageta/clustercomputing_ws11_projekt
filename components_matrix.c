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
#include "list.h"
#include "components.h"

struct node {
	int i, j;
};

static int
visit_node(matrix_type *colors, stack_type *to_be_visited, struct node *node)
{
	int rc = 0;
	unsigned char color;

	color = *((unsigned char *) matrix_get(colors, node->i, node->j));

	/*
	 * push unvisited nodes onto the stack
	 * white nodes are unvisited,
	 * gray nodes already on the stack,
	 * black nodes are already expanded
	 */
	if(!color) { /* white */
		rc = stack_push(to_be_visited, node);
		if(rc)
			goto err_enqueue;

		color = 1; /* gray */
		matrix_set(colors, node->i, node->j, &color);
	}

	return 0;
err_enqueue:
	return rc;
}

static int
complete_component(matrix_type *mat, matrix_type *colors,
		matrix_type *components, stack_type *to_be_visited,
		struct node *search_node, struct component *cur_comp_p)
{
	static stack_type *black_neighbours = 0;

	int i, j, rc = 0;
	struct node node, current_node;
	unsigned char value, color;
	struct component *comp_p;


	if(!cur_comp_p)
		return EINVAL;

	if(!black_neighbours) {
		rc = stack_create(&black_neighbours, sizeof(*search_node));
		if(rc)
			goto err_queue_bn;
	}

	rc = stack_push(black_neighbours, search_node);
	if(rc)
		goto err_enqueue_bn;

	while(stack_size(black_neighbours)) {
		stack_pop(black_neighbours, &current_node);

		for (i = -1; i < 2; i++) {
			/* invalid nods */
			if(!matrix_index_valid(mat, current_node.i + i,
						current_node.j))
				continue;

			for (j = -1; j < 2; j++) {
				/* current node */
				if(!i && !j)
					continue;

				/* invalid nodes */
				if(!matrix_index_valid(mat, current_node.i + i,
							current_node.j + j))
					continue;

				node.i = current_node.i + i;
				node.j = current_node.j + j;

				value = *((unsigned char *)
						matrix_get(mat, node.i,
								node.j));
				comp_p = *((struct component **) matrix_get(
						components, node.i, node.j));

				if(!comp_p && value) {
					cur_comp_p->size += 1;

					matrix_set(components, node.i, node.j,
							&cur_comp_p);
					rc = stack_push(black_neighbours,
							&node);
					if(rc)
						goto err_enqueue_bn;

					color = 2;
					matrix_set(colors, node.i, node.j,
							&color);
				}
				else {
					rc = visit_node(colors, to_be_visited,
							&node);
					if(rc)
						goto err_visit_node;
				}
			}
		}
	}


//	stack_destroy(black_neighbours);

	return 0;
err_visit_node:
err_enqueue_bn:
	stack_destroy(black_neighbours);
err_queue_bn:
	return rc;
}

int
find_components(matrix_type *mat, struct component_list **comp_list)
{
	int i, j, rc = 0;
	unsigned int max_component = 0;
	unsigned char color;
	unsigned char value;
	matrix_type *colors = 0, *components = 0;
	stack_type *to_be_visited = 0;
	list_type *tmp_comp_list;

	struct component comp;
	struct component *comp_p;
	struct component_list *compl_p;
	struct node node, current_node;

	if(!comp_list)
		return EINVAL;

	rc = component_list_create(&compl_p);
	if(rc)
		goto err_compl_create;

	rc = list_create(&tmp_comp_list, sizeof(comp));
	if(rc)
		goto err_list_tcl;

	rc = matrix_copy(&colors, mat);
	if(rc)
		goto err_matrix_colors;
	/*
	 * 0 - white
	 * 1 - gray
	 * 2 - black
	 */
	matrix_init(colors, 0);

	rc = matrix_create(&components, mat->m, mat->n, sizeof(comp_p));
	if(rc)
		goto err_matrix_componts;
	/* 0 - no component */
	matrix_init(components, 0);

	rc = stack_create(&to_be_visited, sizeof(current_node));
	if(rc)
		goto err_queue_tbv;

	/* initial node to expand */
	node.i = 0;
	node.j = 0;
	color = 1;
	matrix_set(colors, node.i, node.j, &color);
	rc = stack_push(to_be_visited, &node);
	if(rc)
		goto err_enqueue_tbv;

	while(stack_size(to_be_visited)) {
		stack_pop(to_be_visited, &current_node);

		color = *((char *) matrix_get(colors, current_node.i, current_node.j));
		if(color > 1)
			continue;

		color = 2;
		matrix_set(colors, current_node.i, current_node.j, &color);

		/* push all valid neighbours onto the stack (expand node) */
		for (i = -1; i < 2; i++) {
			/* invalid nods */
			if(!matrix_index_valid(mat, current_node.i + i,
						current_node.j))
				continue;

			for (j = -1; j < 2; j++) {
				/* current node */
				if(!i && !j)
					continue;

				/* invalid nodes */
				if(!matrix_index_valid(mat, current_node.i + i,
						current_node.j + j))
					continue;

				node.i = current_node.i + i;
				node.j = current_node.j + j;

				value = *((unsigned char *) matrix_get(mat,
						current_node.i, current_node.j));
				/* white nodes need not further handling */
				if(value) {
					/*
					 * in case of black nodes we want to update the
					 * components
					 */
					comp_p = *((struct component **) matrix_get(components,
							current_node.i, current_node.j));

					/*
					 * in case the node is black and has no component,
					 * it is a part of a new component
					 */
					if(!comp_p) {
						memset(&comp, 0, sizeof(comp));
						comp.example_coords[0] = current_node.i;
						comp.example_coords[1] = current_node.j;
						comp.component_id = ++max_component;
						comp.size = 1;

						rc = list_append(tmp_comp_list, &comp);
						if(rc)
							goto err_component_create;

						comp_p = (struct component *)
							list_tail(tmp_comp_list);

						/*
						 * this will ensure that all changes made to
						 * the component referenced in the matrix are
						 * also made to the one in the vector
						 */
						matrix_set(components, current_node.i,
								current_node.j, &comp_p);

						rc = complete_component(mat, colors, components,
								to_be_visited, &current_node,
								comp_p);
						if(rc)
							goto err_complete_comp;
					}
				}

				rc = visit_node(colors, to_be_visited, &node);
				if(rc)
					goto err_visit_node;
			}
		}
	}


	fprintf(stdout, "\ncomponents:\n");
	for (i = 0; i < components->m; i++) {
		for (j = 0; j < (components->n - 1); j++) {
			comp_p = *((struct component **) matrix_get(components,
						i, j));

			if(comp_p)
				fprintf(stdout, "%2u, ", comp_p->component_id);
			else
				fprintf(stdout, "  , ");
		}
		comp_p = *((struct component **) matrix_get(components, i, j));

		if(comp_p)
			fprintf(stdout, "%2u\n", comp_p->component_id);
		else
			fprintf(stdout, "  \n");
	}

	stack_destroy(to_be_visited);
	matrix_destroy(components);
	matrix_destroy(colors);

	rc = vector_append_list(compl_p->components, tmp_comp_list);
	if(rc)
		goto err_vecappend_components;

	list_destroy(tmp_comp_list);

	fprintf(stdout, "\n");
	for(i = 0; i < compl_p->components->elements; i++) {
		comp_p = (struct component *)
			vector_get_value(compl_p->components, i);
		fprintf(stderr, "%u\n", comp_p->size);
	}

	*comp_list = compl_p;

	return 0;
err_complete_comp:
err_visit_node:
err_component_create:
err_enqueue_tbv:
	stack_destroy(to_be_visited);
err_queue_tbv:
	matrix_destroy(components);
err_matrix_componts:
	matrix_destroy(colors);
err_vecappend_components:
err_matrix_colors:
	list_destroy(tmp_comp_list);
err_list_tcl:
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
	if(rc)
		goto err_vector_create;

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
