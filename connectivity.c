#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>

#include "matrix.h"
#include "queue.h"

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
find_connections(matrix_type *mat)
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

static char * usage() {
	static char text[256];
	int written;
	char *textp = text;

	written = sprintf(textp, "usage: connectivity <file>");
	textp += written;

	return text;
}

static int read_input_file(matrix_type **m, char *file_name)
{
	int i, j, rc;
	unsigned int height, width;
	long int read_value;
	size_t line_length;
	unsigned short int write_value;
	char *line = NULL, *endp, *strp;
	FILE *input = NULL;
	matrix_type *matrix;
	queue_type *lines;

	if(queue_create(&lines, sizeof(line))) {
		fprintf(stderr, "Not enougth memory.\n");
		goto err_out;
	}

	input = fopen(file_name, "r");
	if(!input) {
		fprintf(stderr, "file '%s' could not be found.\n",
				file_name);
		goto err_out;
	}

	while (getline(&line, &line_length, input) > 0) {
		if(queue_enqueue(lines, &line)) {
			fprintf(stderr, "Not enougth memory.\n");
			free(line);
			goto err_free_queue;
		}
		line = NULL;
	}
	free(line);

	fclose(input);
	input = NULL;

	line = *((char **) queue_head(lines));
	width = 0;

	while(*line) {
		if((*line == ',') && (*(line+1) == ' '))
			width++;

		if(!width && isdigit(*line))
			width++;

		line++;
	}

	height = queue_size(lines);

	fprintf(stdout, "height: %d; width: %d\n", height, width);

	rc = matrix_create(&matrix, height, width, sizeof(unsigned short int));
	if(rc) {
		fprintf(stderr, "Could not create a matrix.. %s\n",
				strerror(rc));
		goto err_free_queue;
	}
	matrix_init(matrix, 0);

	i = 0;
	while (queue_size(lines)) {
		queue_dequeue(lines, &line);

		j = 0;
		strp = line;

		strp = strtok(line, ", \n");
		while(strp && (j < width) && (i < height)) {
			read_value = strtol(strp, &endp, 10);

			if((errno == ERANGE) || (strp == endp)) {
				fprintf(stderr, "file '%s' contains invalid"
						" input.\n",
						file_name);
				free(line);
				goto err_free_queue;
			}

			write_value = (read_value ? 1 : 0);
			matrix_set(matrix, i, j, &write_value);
			j++;

			strp = strtok(NULL, ", \n");
		}

		i++;
		free(line);
	}
	queue_destroy(lines);

	*m = matrix;
	return 0;
err_free_queue:
	while(queue_size(lines)) {
		queue_dequeue(lines, &line);
		free(line);
	}
	queue_destroy(lines);
err_out:
	if(input)
		fclose(input);
	return -1;
}

int
main(int argc, char ** argv)
{
	int i,j, rc;
	matrix_type *matrix;

	if(argc < 2) {
		fprintf(stderr, "To few arguments given.\n\n%s\n",
				usage(usage()));
		return -1;
	}

	if(read_input_file(&matrix, argv[1])) {
		fprintf(stderr, "\n%s\n", usage());
		return -1;
	}

	fprintf(stdout, "test_matrix:\n");
	for (i = 0; i < matrix->m; i++) {
		for (j = 0; j < (matrix->n - 1); j++) {
			fprintf(stdout, "%hd, ", *((unsigned short int *)
						matrix_get(matrix, i, j)));
		}
		fprintf(stdout, "%hd\n", *((unsigned short int *)
					matrix_get(matrix, i, j)));
	}

	rc = find_connections(matrix);

	matrix_destroy(matrix);

	return rc;
}
