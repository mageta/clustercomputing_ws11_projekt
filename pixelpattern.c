#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <wchar.h>

#include "matrix.h"
#include "stack.h"
#include "components.h"

int generate_pixel_pattern(unsigned long int height, unsigned long int width,
		double filling, unsigned int failcount, unsigned int bulginess,
		unsigned int size, unsigned int dist);

static char * usage(int argc, char ** argv) {
	static char text[256];
	int written;
	char *textp = text;

	written = sprintf(textp, "usage: %s <height> <width>", argv[0]);
	textp += written;

	return text;
}

long int read_input(char * arg) {
	long int val;
	char *endp;

	errno = 0;
	val = strtol(arg, &endp, 10);

	if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) ||
			(errno != 0 && val == 0)) {
		val = -1;
		goto err_out;
	}

	if (endp == arg) {
		val = -2;
		goto err_out;
	}

err_out:
	return val;
}

int
main(int argc, char ** argv)
{
	long int width, height;
	double filling = 0.3;
	unsigned int failcount = 500,
		     bulginess = 4,
		     size = 10,
		     dist = 1;

	if(argc < 3) {
		fprintf(stderr, "To few arguments given.\n\n%s\n",
				usage(argc, argv));
		return -1;
	}

	width = read_input(argv[2]);
	if(width < 1) {
		fprintf(stderr, "Invalid width-argument.\n\n%s\n",
				usage(argc, argv));
		return -1;
	}

	height = read_input(argv[1]);
	if(height < 1) {
		fprintf(stderr, "Invalid height-argument.\n\n%s\n",
				usage(argc, argv));
		return -1;
	}

	return generate_pixel_pattern(height, width, filling, failcount,
			bulginess, size, dist);
}

struct node {
	unsigned long int i, j;
};

static int
distance_test(matrix_type *matrix, matrix_type *components,
		struct node *node, unsigned int dist, unsigned int value,
		struct component *cur_comp_p, int count)
{
	int i, j, k = 0;
	int cur_i, cur_j;
	long long distl = dist;
	unsigned char cur_value;

	struct component *comp_p;

	if(dist < 1)
		return *((unsigned char *) matrix_get(matrix, node->i, node->j)) == value;

	for (i = (-1 * distl); i < (distl + 1); i++) {
		/* invalid nods */
		if(!matrix_index_valid(matrix, node->i + i, node->j))
			continue;

		for (j = (-1 * distl); j < (distl + 1); j++) {
			cur_i = node->i + i;
			cur_j = node->j + j;

			/* invalid nodes */
			if(!matrix_index_valid(matrix, cur_i, cur_j))
				continue;

			cur_value = *((unsigned char *) matrix_get(matrix, cur_i, cur_j));
			comp_p = *((struct component **) matrix_get(components, cur_i, cur_j));

			if(cur_value == value) {
				if(comp_p == cur_comp_p) {
					k++;
					if(k >= count)
						return 1;
				} else {
					return 1;
				}
			}
		}
	}

	return 0;
}

int generate_pixel_pattern(unsigned long int height, unsigned long int width,
		double filling, unsigned int failcount, unsigned int bulginess,
		unsigned int size, unsigned int dist)
{
	int i, j, prob, dice, rc;
	unsigned char value, color;
	unsigned long long int filled;
	unsigned int max_component = 0, failed;

	struct component *comp_p;

	matrix_type *matrix; /* contains unsigned short int */
	matrix_type *colors; /* contains unsigned short int */
	matrix_type *components; /* contains (struct component *) */
	stack_type *to_be_visited;

	struct node node, current_node;

	matrix_create(&matrix, height, width, sizeof(value));
	matrix_init(matrix, 0);

	matrix_create(&colors, height, width, sizeof(color));
	matrix_init(colors, 0);

	matrix_create(&components, height, width, sizeof(comp_p));
	matrix_init(components, 0);

	stack_create(&to_be_visited, sizeof(node));

	srand(time(NULL));

	filled = 0;
	failed = 0;

	/* TODO: calculation is strange */
	while (((double) filled / (height * width)) < filling) {
		i = rand() % height;
		j = rand() % width;

		node.i = i;
		node.j = j;
		value = 1;

		/* look for an other component nearby */
		rc = distance_test(matrix, components, &node, dist, value, NULL, 0);
		if(rc) { /* a other component is too close */
			failed++;

			/* we failed too often */
			if (failed >= failcount)
				break;
			continue;
		}

		failed = 0;

		/* create a new component */
		component_create(&comp_p);
		comp_p->size = 1;
		comp_p->example_coords[0] = node.i;
		comp_p->example_coords[1] = node.j;
		comp_p->component_id = ++max_component;

		matrix_set(matrix, node.i, node.j, &value);
		matrix_set(components, node.i, node.j, &comp_p);

		stack_push(to_be_visited, &node);

		while(stack_size(to_be_visited)) {
			stack_pop(to_be_visited, &current_node);

			color = 2; /* color it black */
			matrix_set(colors, current_node.i, current_node.j, &color);

			for (i = -1; i < 2; i++) {
				/* invalid nods */
				if(!matrix_index_valid(matrix, current_node.i + i, current_node.j))
					continue;

				for (j = -1; j < 2; j++) {
					/* current node */
					if(!i && !j)
						continue;

					/* invalid nodes */
					if(!matrix_index_valid(matrix, current_node.i + i, current_node.j + j))
						continue;

					node.i = current_node.i + i;
					node.j = current_node.j + j;

					color = *((unsigned char *) matrix_get(colors, node.i, node.j));
					if(color) /* if not white */
						continue;

					color = 1; /* color it gray */
					matrix_set(colors, node.i, node.j, &color);

					rc = distance_test(matrix, components, &node, dist, value, comp_p, bulginess);
					if(rc) /* if the dist to some other comp is too short */
						continue;

					/* the bigger the components get, the smaller gets the probability that it gets an other neighboring */
					prob = 100 - (comp_p->size * size);
					if(prob < 0) prob = 0;
					dice = (rand() % 100) + 1;

					if(dice <= prob) {
						value = 1;

						comp_p->size += 1;

						matrix_set(components, node.i, node.j, &comp_p);
						matrix_set(matrix, node.i, node.j, &value);

						stack_push(to_be_visited, &node);
						filled++;

						if(((double) filled / (height * width)) >= filling)
							goto fill_out;
					}
				}
			}
		}
	}
fill_out:

	fprintf(stdout, "components:\n");
	for (i = 0; i < components->m; i++) {
		for (j = 0; j < (components->n - 1); j++) {
			comp_p = *((struct component **) matrix_get(components, i, j));

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

	fprintf(stdout, "pattern_size:\n");
	for (i = 0; i < components->m; i++) {
		for (j = 0; j < (components->n - 1); j++) {
			comp_p = *((struct component **) matrix_get(components, i, j));

			if(comp_p)
				fprintf(stdout, "%2d, ", comp_p->size);
			else
				fprintf(stdout, "  , ");
		}
		comp_p = *((struct component **) matrix_get(components, i, j));

		if(comp_p)
			fprintf(stdout, "%2d\n", comp_p->size);
		else
			fprintf(stdout, "  \n");
	}

	fprintf(stdout, "values:\n");
	for (i = 0; i < matrix->m; i++) {
		for (j = 0; j < (matrix->n - 1); j++) {
			fprintf(stderr, "%hhd, ", *((unsigned short int *)
						matrix_get(matrix, i, j)));
		}
		fprintf(stderr, "%hhd\n", *((unsigned short int *)
					matrix_get(matrix, i, j)));
	}

	fprintf(stdout, "filled: %lld; max: %ld; filling: %f; components: %d\n", filled,
			height * width, ((double) filled) / (height * width), max_component);

	matrix_destroy(matrix);
	matrix_destroy(colors);
	for (i = 0; i < components->m; i++) {
		for (j = 0; j < (components->n - 1); j++) {
			comp_p = *((struct component **) matrix_get(components, i, j));
			if(comp_p)
				free(comp_p);
		}
	}
	matrix_destroy(components);
	stack_destroy(to_be_visited);

	return 0;
}
