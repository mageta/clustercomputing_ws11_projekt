#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>

#include "matrix.h"
#include "queue.h"
#include "components.h"

struct node {
	int i, j;
};

static void
visit_node(matrix_type *colors, queue_type *to_be_visited, struct node *node)
{
	unsigned char color;

	color = *((unsigned char *) matrix_get(colors, node->i, node->j));

	/*
	 * push unvisited nodes onto the stack
	 * white nodes are unvisited,
	 * gray nodes already on the stack,
	 * black nodes are already expanded
	 */
	if(!color) { /* white */
		queue_enqueue(to_be_visited, node);

		color = 1; /* gray */
		matrix_set(colors, node->i, node->j, &color);
	}
}

static int
complete_component(matrix_type *mat, matrix_type *colors,
		matrix_type *components, queue_type *to_be_visited,
		struct node *search_node, struct component *cur_comp_p)
{
	int i, j;
	struct node node, current_node;
	unsigned char value;
	struct component *comp_p;

	queue_type *black_neighbours;

	if(!cur_comp_p)
		return EINVAL;

	queue_create(&black_neighbours, sizeof(*search_node));
	queue_enqueue(black_neighbours, search_node);

	while(queue_size(black_neighbours)) {
		queue_dequeue(black_neighbours, &current_node);

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
					queue_enqueue(black_neighbours, &node);
				}

				visit_node(colors, to_be_visited, &node);
			}
		}
	}

	queue_destroy(black_neighbours);

	return 0;
}

int
find_components(matrix_type *mat)
{
	int i, j;
	unsigned int max_component = 0;
	unsigned char color;
	unsigned char value;
	matrix_type *colors = 0, *components = 0;
	queue_type *to_be_visited = 0;

	struct component *comp_p;
	struct node node, current_node;

	matrix_copy(&colors, mat);
	/*
	 * 0 - white
	 * 1 - gray
	 * 2 - black
	 */
	matrix_init(colors, 0);

	matrix_create(&components, mat->m, mat->n, sizeof(comp_p));
	/* 0 - no component */
	matrix_init(components, 0);

	queue_create(&to_be_visited, sizeof(current_node));

	/* initial node to expand */
	node.i = 0;
	node.j = 0;
	color = 1;
	matrix_set(colors, node.i, node.j, &color);
	queue_enqueue(to_be_visited, &node);

	while(queue_size(to_be_visited)) {
		queue_dequeue(to_be_visited, &current_node);

		color = 2;
		matrix_set(colors, current_node.i, current_node.j, &color);

		value = *((unsigned char *) matrix_get(mat,
				current_node.i, current_node.j));
		/* white nodes need not further handling */
		if(!value) {
			comp_p = 0;
		} else {
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
				component_create(&comp_p);

				comp_p->example_coords[0] = current_node.i;
				comp_p->example_coords[1] = current_node.j;
				comp_p->component_id = ++max_component;
				comp_p->size = 1;

				matrix_set(components, current_node.i,
						current_node.j, &comp_p);

				complete_component(mat, colors, components,
						to_be_visited, &current_node,
						comp_p);
			}
		}

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

				visit_node(colors, to_be_visited, &node);
			}
		}
	}


	fprintf(stdout, "\ncomponents:\n");
	for (i = 0; i < components->m; i++) {
		for (j = 0; j < (components->n - 1); j++) {
			comp_p = *((struct component **) matrix_get(components,
						i, j));

			if(comp_p)
				fprintf(stdout, "%2d, ", comp_p->component_id);
			else
				fprintf(stdout, "  , ");
		}
		comp_p = *((struct component **) matrix_get(components, i, j));

		if(comp_p)
			fprintf(stdout, "%2d\n", comp_p->component_id);
		else
			fprintf(stdout, "  \n");
	}

	queue_destroy(to_be_visited);
	matrix_destroy(components);
	matrix_destroy(colors);

	return 0;
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
