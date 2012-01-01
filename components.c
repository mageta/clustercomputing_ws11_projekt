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
	unsigned short int color;

	color = *((unsigned short int *) matrix_get(colors, node->i, node->j));

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
		struct node *search_node, unsigned short int cur_component)
{
	int i, j;
	struct node node, current_node;
	unsigned short int value, component;

	queue_type *black_neighbours;

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

				value = *((unsigned short int *)
						matrix_get(mat, node.i,
								node.j));
				component = *((unsigned short int *)
						matrix_get(components, node.i,
								node.j));

				if(cur_component && !component && value) {
					component = cur_component;

					matrix_set(components, node.i, node.j,
							&component);

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
	unsigned short int color;
	unsigned short int max_component = 0, cur_component;
	unsigned short int value;
	matrix_type *colors = 0, *components = 0;
	queue_type *to_be_visited = 0;

	struct node node, current_node;

	matrix_copy(&colors, mat);
	/*
	 * 0 - white
	 * 1 - gray
	 * 2 - black
	 */
	matrix_init(colors, 0);

	matrix_copy(&components, mat);
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

		value = *((unsigned short int *) matrix_get(mat,
				current_node.i, current_node.j));
		/* white nodes need not further handling */
		if(!value) {
			cur_component = 0;
		} else {
			/*
			 * in case of black nodes we want to update the
			 * components
			 */
			cur_component =
				*((unsigned short int *) matrix_get(components,
					current_node.i, current_node.j));

			/*
			 * in case the node is black and has no component,
			 * it is a part of a new component
			 */
			if(!cur_component) {
				max_component++;
				matrix_set(components, current_node.i,
						current_node.j,	&max_component);
				cur_component = max_component;

				complete_component(mat, colors, components,
						to_be_visited, &current_node,
						cur_component);
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
		unsigned short int k;

		for (j = 0; j < (components->n - 1); j++) {
			k = *((unsigned short int *)
					matrix_get(components, i, j));

			if(k)
				fprintf(stdout, "%2hd, ", k);
			else
				fprintf(stdout, "  , ");
		}
		k = *((unsigned short int *) matrix_get(components, i, j));

		if(k)
			fprintf(stdout, "%2hd\n", k);
		else
			fprintf(stdout, "  \n");
	}

	queue_destroy(to_be_visited);
	matrix_destroy(components);
	matrix_destroy(colors);

	return 0;
}
