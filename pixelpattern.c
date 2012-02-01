#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <wchar.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "matrix.h"
#include "stack.h"
#include "components.h"
#include "constvector.h"
#include "queue.h"

int generate_pixel_pattern(unsigned long int height, unsigned long int width,
		double filling, unsigned int failcount, unsigned int bulginess,
		double size, unsigned int dist);

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

static unsigned int urand()
{
	int urandom = open("/dev/urandom", O_RDONLY), rlen;
	unsigned int rc;

	assert(urandom >= 0);

	rlen = read(urandom, &rc, sizeof(rc));

	assert(rlen == sizeof(rc));

	close(urandom);

	return rc;
}

int
main(int argc, char ** argv)
{
	/* read from argv */
	long int width, height;

	/* please see the project-documentation for detailed explanations */
	double filling = 0.5;
	unsigned int failcount = 500,
		     bulginess = 3,
		     dist = 1;
	double	     size = 5;

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
		double size, unsigned int dist)
{
	int i, j, rc;
	double prob, dice;
	unsigned char value, color;
	unsigned long long int filled;
	unsigned int max_component = 0, failed;

	struct component comp;
	struct component *comp_p;

	matrix_type *matrix; /* contains unsigned short int */
	matrix_type *colors; /* contains unsigned short int */
	matrix_type *components; /* contains (struct component *) */
	stack_type *to_be_visited;
//	queue_type *to_be_visited;
	constvector_type *comp_list;
//	list_type *comp_list;

	struct node node, current_node;

	rc = matrix_create(&matrix, height, width, sizeof(value));
	if(rc)
		goto err_matrix_matrix;

	rc = matrix_create(&colors, height, width, sizeof(color));
	if(rc)
		goto err_matrix_colors;

	rc = matrix_create(&components, height, width, sizeof(comp_p));
	if(rc)
		goto err_matrix_components;

	rc = stack_create(&to_be_visited, sizeof(node));
//	rc = queue_create(&to_be_visited, sizeof(node));
	if(rc)
		goto err_stack_tbv;

	rc = constvector_create(&comp_list, 0, sizeof(comp));
//	rc = list_create(&comp_list, sizeof(comp));
	if(rc)
		goto err_list_cl;

	srand(urand());

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
		memset(&comp, 0, sizeof(comp));
		comp.size = 1;
		comp.example_coords[0] = node.i;
		comp.example_coords[1] = node.j;
		comp.component_id = ++max_component;

		rc = constvector_add(comp_list, &comp);
//		rc = list_append(comp_list, &comp);
		if(rc)
			goto err_append_cl;

		comp_p = (struct component *) constvector_element(comp_list, comp_list->elements - 1);
//		comp_p = (struct component *) list_tail(comp_list);

		matrix_set(matrix, node.i, node.j, &value);
		matrix_set(components, node.i, node.j, &comp_p);

		rc = stack_push(to_be_visited, &node);
//		rc = queue_enqueue(to_be_visited, &node);
		if(rc)
			goto err_push_tbv;

		while(stack_size(to_be_visited)) {
//		while(queue_size(to_be_visited)) {
			stack_pop(to_be_visited, &current_node);
//			queue_dequeue(to_be_visited, &current_node);

			color = *((unsigned char *) matrix_get(colors, current_node.i, current_node.j));
			if(color > 1)
				continue;

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

					/* the bigger the components get, the smaller gets the probability that it gets an other neighbour */
					prob = 100 - (comp_p->size * size);
					if(prob < 0) prob = 0;
					dice = (rand() % 100) + 1;

					if(dice <= prob) {
						value = 1;

						comp_p->size += 1;

						matrix_set(components, node.i, node.j, &comp_p);
						matrix_set(matrix, node.i, node.j, &value);

						rc = stack_push(to_be_visited, &node);
//						rc = queue_enqueue(to_be_visited, &node);
						if(rc)
							goto err_push_tbv;

						filled++;

						if(((double) filled / (height * width)) >= filling)
							goto fill_out;
					}
					else {
						color = 2;
						matrix_set(colors, node.i, node.j, &color);
					}
				}
			}
		}
	}
fill_out:
	for (i = 0; i < matrix->m; i++) {
		for (j = 0; j < (matrix->n - 1); j++) {
			fprintf(stderr, "%hhd, ", *((unsigned char *)
						matrix_get(matrix, i, j)));
		}
		fprintf(stderr, "%hhd\n", *((unsigned char *)
					matrix_get(matrix, i, j)));
	}

	fprintf(stdout, "filled: %lld; max: %ld; filling: %f; components: %d\n", filled,
			height * width, ((double) filled) / (height * width), max_component);

	fprintf(stdout, "\n");
	for(i = 0; i < comp_list->elements; i++) {
		comp_p = (struct component *) constvector_element(comp_list, i);
//		comp_p = (struct component *) list_element(comp_list, i);
		fprintf(stdout, "%u\n", comp_p->size);
	}

	matrix_destroy(matrix);
	matrix_destroy(colors);
	matrix_destroy(components);
	stack_destroy(to_be_visited);
//	queue_destroy(to_be_visited);
	constvector_destroy(comp_list);
//	list_destroy(comp_list);

	return 0;
err_push_tbv:
err_append_cl:
	constvector_destroy(comp_list);
//	list_destroy(comp_list);
err_list_cl:
	stack_destroy(to_be_visited);
//	queue_destroy(to_be_visited);
err_stack_tbv:
	matrix_destroy(components);
err_matrix_components:
	matrix_destroy(colors);
err_matrix_colors:
	matrix_destroy(matrix);
err_matrix_matrix:
	return rc;
}
