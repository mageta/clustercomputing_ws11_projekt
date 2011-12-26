#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <wchar.h>

#include "matrix.h"
#include "stack.h"

int generate_pixel_pattern(unsigned long int height, unsigned long int width,
		double filling, int failcount, int bulginess, int size,
		int dist);

static char * usage() {
	static char text[256];
	int written;
	char *textp = text;

	written = sprintf(textp, "usage: pixelpattern <height> <width>");
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
	int failcount = 500,
	    bulginess = 9,
	    size = 10,
	    dist = 1;

	if(argc < 3) {
		fprintf(stderr, "To few arguments given.\n\n%s\n", usage());
		return -1;
	}

	width = read_input(argv[2]);
	if(width < 1) {
		fprintf(stderr, "Invalid width-argument.\n\n%s\n", usage());
		return -1;
	}

	height = read_input(argv[1]);
	if(height < 1) {
		fprintf(stderr, "Invalid height-argument.\n\n%s\n", usage());
		return -1;
	}

	return generate_pixel_pattern(height, width, filling, failcount,
			bulginess, size, dist);
}

static int debug_hock() {
	return 0;
}

struct node {
	unsigned long int i, j;
};

static int
distance_test(matrix_type *matrix, matrix_type *components,
		struct node *node, unsigned int dist, unsigned short int value,
		long long target_component, int count)
{
	int i, j, k = 0;
	int cur_i, cur_j;
	unsigned short int cur_value;
	unsigned int component;
	long long distl = dist;

	if(dist < 1)
		return *((unsigned short int *) matrix_get(matrix, node->i,
					node->j)) == value;

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

			cur_value = *((unsigned short int *) matrix_get(matrix,
						cur_i, cur_j));
			component = *((unsigned int *) matrix_get(components,
						cur_i, cur_j));

			if((component != target_component) &&
					(cur_value == value))
				return 1;

			if(cur_value == value) {
				k++;
				if(k >= count)
					return 1;
			}
		}
	}

	return 0;
}

static int
update_component(matrix_type *components, matrix_type *pattern_sizes,
		struct node *target_node, unsigned int target_component,
		unsigned long long int new_size)
{
	int i, j;
	struct node node, current_node;
	unsigned int component, base_component;
	unsigned long long int size;

	stack_type *comp_neighbours;

	stack_create(&comp_neighbours, sizeof(node));
	stack_push(comp_neighbours, target_node);

	base_component = *((unsigned int *) matrix_get(components,
				target_node->i, target_node->j));

	matrix_set(pattern_sizes, target_node->i, target_node->j, &new_size);
	matrix_set(components, target_node->i, target_node->j,
			&target_component);

	while(stack_size(comp_neighbours)) {
		stack_pop(comp_neighbours, &current_node);

		for (i = -1; i < 2; i++) {
			/* invalid nods */
			if(!matrix_index_valid(components, current_node.i + i,
						current_node.j))
				continue;

			for (j = -1; j < 2; j++) {
				/* current node */
				if(!i && !j)
					continue;

				/* invalid nodes */
				if(!matrix_index_valid(components,
							current_node.i + i,
							current_node.j + j))
					continue;

				node.i = current_node.i + i;
				node.j = current_node.j + j;

				component = *((unsigned int *)
						matrix_get(components, node.i,
								node.j));
				size = *((unsigned int *)
						matrix_get(pattern_sizes,
							node.i,	node.j));

				if(component == base_component) {
					matrix_set(pattern_sizes,
							node.i, node.j,
							&new_size);
					matrix_set(components,
							node.i, node.j,
							&target_component);

					if(size != new_size)
						stack_push(comp_neighbours,
								&node);
				}
			}
		}
	}

	stack_destroy(comp_neighbours);

	return 0;
}

static int change_pattern_value(matrix_type *matrix, matrix_type *pattern_sizes,
		matrix_type *components, struct node *target_node,
		unsigned short int target_value)
{
	int i, j, first_find = 1;
	unsigned short int value, cur_value;
	unsigned long long int target_pattern_size = 1, pattern_size;
	unsigned int component, cur_component, merge_component = 0;
	static unsigned int max_component = 1;

	struct node node, current_node;
	stack_type *same_value;

	stack_create(&same_value, sizeof(*target_node));

	matrix_set(matrix, target_node->i, target_node->j, &target_value);
	matrix_set(pattern_sizes, target_node->i, target_node->j,
			&target_pattern_size);
	stack_push(same_value, target_node);

	while(stack_size(same_value)) {
		stack_pop(same_value, &current_node);

		cur_value = *((unsigned short int *) matrix_get(matrix,
					current_node.i, current_node.j));

		cur_component = *((unsigned int *) matrix_get(components,
					current_node.i, current_node.j));

		for (i = -1; i < 2; i++) {
			/* invalid nods */
			if(!matrix_index_valid(matrix, current_node.i + i,
						current_node.j))
				continue;

			for (j = -1; j < 2; j++) {
				/* current node */
				if(!i && !j)
					continue;

				/* invalid nodes */
				if(!matrix_index_valid(matrix,
							current_node.i + i,
							current_node.j + j))
					continue;

				node.i = current_node.i + i;
				node.j = current_node.j + j;

				value = *((unsigned short int *) matrix_get(
							matrix, node.i,
							node.j));

				pattern_size = *((unsigned long long int *)
						matrix_get(pattern_sizes,
							node.i, node.j));

				component = *((unsigned int *)
						matrix_get(components,
							node.i, node.j));

				if(value != target_value)
					continue;

				if(first_find) {
					merge_component = component;

					target_pattern_size = pattern_size + 1;

					matrix_set(components, current_node.i,
							current_node.j,
							&merge_component);

					update_component(components,
							pattern_sizes,
							&node,
							merge_component,
							target_pattern_size);

					first_find = 0;
				}

				if(component != merge_component) {
					target_pattern_size += pattern_size;

					update_component(components,
							pattern_sizes,
							&node,
							merge_component,
							target_pattern_size);
					update_component(components,
							pattern_sizes,
							&current_node,
							merge_component,
							target_pattern_size);
				}
			}
		}

		if(first_find) {
			max_component++;
			merge_component = max_component;

			matrix_set(components, current_node.i, current_node.j,
					&merge_component);

			first_find = 0;
		}
	}

	stack_destroy(same_value);

	return 0;
}

int generate_pixel_pattern(unsigned long int height, unsigned long int width,
		double filling, int failcount, int bulginess, int size,
		int dist)
{
	int i, j;
	int prob;
	int dice;
	unsigned short int value, color;
	unsigned long long int pattern_size, filled, failed;
	unsigned int component, max_component = 0;
	matrix_type *matrix; /* contains unsigned short int */
	matrix_type *colors; /* contains unsigned short int */
	matrix_type *pattern_sizes; /* contains unsigned long long int */
	matrix_type *components; /* contains unsigned int */
	stack_type *to_be_visited;

	struct node node, current_node;

	matrix_create(&matrix, height, width, sizeof(value));
	matrix_init(matrix, 0);

	matrix_create(&colors, height, width, sizeof(color));
	matrix_init(colors, 0);

	matrix_create(&components, height, width, sizeof(component));
	matrix_init(components, 0);

	matrix_create(&pattern_sizes, height, width, sizeof(pattern_size));
	matrix_init(pattern_sizes, 0);

	stack_create(&to_be_visited, sizeof(node));

	srand(time(NULL));

	filled = 0;
	failed = 0;

	while (((double) filled / (height * width)) < filling) {
		i = rand() % height;
		j = rand() % width;

		node.i = i;
		node.j = j;
		value = 1;

		if(distance_test(matrix, components, &node, dist, value, -1,
					0)) {
			failed++;
			if (failed >= failcount)
				break;
			continue;
		}

		failed = 0;

		component = ++max_component;
		pattern_size = 1;

		matrix_set(matrix, node.i, node.j, &value);
		matrix_set(components, node.i, node.j, &component);
		matrix_set(pattern_sizes, node.i, node.j, &pattern_size);

		stack_push(to_be_visited, &node);

		while(stack_size(to_be_visited)) {
			stack_pop(to_be_visited, &current_node);

			color = 2;
			matrix_set(colors, current_node.i, current_node.j,
					&color);

			for (i = -1; i < 2; i++) {
				/* invalid nods */
				if(!matrix_index_valid(matrix,
							current_node.i + i,
							current_node.j))
					continue;

				for (j = -1; j < 2; j++) {
					/* current node */
					if(!i && !j)
						continue;

					/* invalid nodes */
					if(!matrix_index_valid(matrix,
							current_node.i + i,
							current_node.j + j))
						continue;

					debug_hock();

					node.i = current_node.i + i;
					node.j = current_node.j + j;

					color = *((unsigned short int *)
							matrix_get(colors,
								node.i,
								node.j));
					if(color)
						continue;

					color = 1;
					matrix_set(colors, node.i, node.j,
							&color);

					if(distance_test(matrix, components,
								&node, dist,
								value,
								component,
								bulginess))
						continue;

					prob = 100 - (pattern_size * size);
					if(prob < 0) prob = 0;
					dice = (rand() % 100) + 1;

					if(dice <= prob) {
						value = 1;
						change_pattern_value(matrix,
								pattern_sizes,
								components,
								&node, value);

						pattern_size =
						   *((unsigned long long int *)
							matrix_get(
								pattern_sizes,
								node.i,
								node.j));

						stack_push(to_be_visited,
								&node);
						filled++;

						if(((double) filled /
							(height * width)) >=
								filling)
							goto fill_out;
					}
				}
			}
		}

		matrix_init(colors, 0);
	}
fill_out:

	fprintf(stdout, "components:\n");
	for (i = 0; i < components->m; i++) {
		for (j = 0; j < (components->n - 1); j++) {
			fprintf(stdout, "%2d, ", *((unsigned int *)
						matrix_get(components, i, j)));
		}
		fprintf(stdout, "%2d\n", *((unsigned int *)
					matrix_get(components, i, j)));
	}

	fprintf(stdout, "pattern_sizes:\n");
	for (i = 0; i < pattern_sizes->m; i++) {
		for (j = 0; j < (pattern_sizes->n - 1); j++) {
			fprintf(stdout, "%3lld, ", *((unsigned long long int *)
						matrix_get(pattern_sizes, i,
							j)));
		}
		fprintf(stdout, "%3lld\n", *((unsigned long long int *)
					matrix_get(pattern_sizes, i, j)));
	}

	fprintf(stdout, "values:\n");
	for (i = 0; i < matrix->m; i++) {
		for (j = 0; j < (matrix->n - 1); j++) {
			fprintf(stderr, "%hd, ", *((unsigned short int *)
						matrix_get(matrix, i, j)));
		}
		fprintf(stderr, "%hd\n", *((unsigned short int *)
					matrix_get(matrix, i, j)));
	}

	fprintf(stdout, "filled: %lld; max: %ld; filling: %f\n\n", filled,
			height * width, ((double) filled) / (height * width));

	for (i = 0; i < matrix->m; i++) {
		unsigned short int k;

		for (j = 0; j < (matrix->n - 1); j++) {
			k = *((unsigned short int *) matrix_get(matrix, i, j));

			if(k)
				putchar('1');
			else
				putchar(' ');
		}
		k = *((unsigned short int *) matrix_get(matrix, i, j));

		if(k)
			putchar('1');
		else
			putchar(' ');
		putchar('\n');
	}

	matrix_destroy(matrix);
	matrix_destroy(colors);
	matrix_destroy(pattern_sizes);
	matrix_destroy(components);
	stack_destroy(to_be_visited);

	return 0;
}
