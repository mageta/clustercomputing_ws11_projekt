#include "matrixgraph.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "matrix.h"

#define TEST_M 10
#define TEST_N 10

int
main(int argc, char ** argv)
{
	int numbers[TEST_M][TEST_N];
	int i, j, rc;

	int val;
	char * strneighbors;
	struct matrix_graph_neighbor_iterator *git;

	matrix_graph_type *graph;
	matrix_graph_node_type node;

	srand(time(NULL));

	for(i = 0; i < TEST_M; i++) {
		for(j = 0; j < TEST_N; j++) {
			numbers[i][j] = rand() % (TEST_M * TEST_N * 100);
		}
	}

	rc = matrix_graph_create(&graph, TEST_M, TEST_N, sizeof(val));
	if(rc) {
		fprintf(stderr, "couldn't create a graph.. %s\n", strerror(rc));
		return rc;
	}

	val = 0;
	rc = matrix_graph_init(graph, &val);
	if(rc) {
		fprintf(stderr, "couldn't init the graph.. %s\n", strerror(rc));
		goto err_free_graph;
	}


	for(i = 0; i < graph->matrix->m; i++) {
		for(j = 0; j < (graph->matrix->n - 1); j++) {
			val = *((int *) matrix_graph_get_node(graph, i, j));
			strneighbors = matrix_graph_strneighbors(graph, i, j);

			fprintf(stdout, "[%d, %s], ", val, strneighbors);
		}

		val = *((int *) matrix_graph_get_node(graph, i, j));
		strneighbors = matrix_graph_strneighbors(graph, i, j);

		fprintf(stdout, "[%d, %s]\n\n", val, strneighbors);
	}

	rc = matrix_graph_neighbor_iterator_create(&git, graph, 0, 0);
	if(rc) {
		fprintf(stderr, "couldn't create a new iterator.. %s\n", strerror(rc));
		goto err_free_graph;
	}

	while(matrix_graph_neighbor_iterator_has_next(git)) {
		rc = matrix_graph_neighbor_iterator_next(git, &node);
		if(rc) {
			fprintf(stderr, "couldn't get the new node ..%s\n", strerror(rc));
			goto err_free_git;
		}

		val = *((int *) node.mem);
		fprintf(stdout, "[(%2d, %2d), %2d)]\n", (unsigned int) node.i, (unsigned int) node.j, val);
	}
	fprintf(stdout, "\n");

	rc = matrix_graph_neighbor_iterator_reinit(git, graph, 1, 1);
	if(rc) {
		fprintf(stderr, "couldn't reinit the iterator.. %s\n", strerror(rc));
		goto err_free_graph;
	}

	while(matrix_graph_neighbor_iterator_has_next(git)) {
		rc = matrix_graph_neighbor_iterator_next(git, &node);
		if(rc) {
			fprintf(stderr, "couldn't get the new node ..%s\n", strerror(rc));
			goto err_free_git;
		}

		val = *((int *) node.mem);
		fprintf(stdout, "[(%2d, %2d), %2d)]\n", (unsigned int) node.i, (unsigned int) node.j, val);
	}
	fprintf(stdout, "\n");

	matrix_graph_del_neighbor(graph, 1, 1, NODE_MIDDLE_LEFT);
	matrix_graph_del_neighbor(graph, 1, 1, NODE_MIDDLE_RIGHT);
	matrix_graph_del_neighbor(graph, 1, 1, NODE_BOTTOM_LEFT);
	matrix_graph_del_neighbor(graph, 1, 1, NODE_BOTTOM_RIGHT);

	rc = matrix_graph_neighbor_iterator_reinit(git, graph, 1, 1);
	if(rc) {
		fprintf(stderr, "couldn't reinit the iterator.. %s\n", strerror(rc));
		goto err_free_graph;
	}

	while(matrix_graph_neighbor_iterator_has_next(git)) {
		rc = matrix_graph_neighbor_iterator_next(git, &node);
		if(rc) {
			fprintf(stderr, "couldn't get the new node ..%s\n", strerror(rc));
			goto err_free_git;
		}

		val = *((int *) node.mem);
		fprintf(stdout, "[(%2d, %2d), %2d)]\n", (unsigned int) node.i, (unsigned int) node.j, val);
	}
	fprintf(stdout, "\n");

	matrix_graph_del_neighbor(graph, 1, 1, NODE_TOP_CENTER);
	matrix_graph_del_neighbor(graph, 1, 1, NODE_BOTTOM_CENTER);
	matrix_graph_add_neighbor(graph, 1, 1, NODE_BOTTOM_LEFT);
	matrix_graph_add_neighbor(graph, 1, 1, NODE_BOTTOM_RIGHT);

	rc = matrix_graph_neighbor_iterator_reinit(git, graph, 1, 1);
	if(rc) {
		fprintf(stderr, "couldn't reinit the iterator.. %s\n", strerror(rc));
		goto err_free_graph;
	}

	while(matrix_graph_neighbor_iterator_has_next(git)) {
		rc = matrix_graph_neighbor_iterator_next(git, &node);
		if(rc) {
			fprintf(stderr, "couldn't get the new node ..%s\n", strerror(rc));
			goto err_free_git;
		}

		val = *((int *) node.mem);
		fprintf(stdout, "[(%2d, %2d), %2d)]\n", (unsigned int) node.i, (unsigned int) node.j, val);
	}

	matrix_graph_destroy(graph);
	matrix_graph_neighbor_iterator_destroy(git);

	return 0;
err_free_git:
	matrix_graph_neighbor_iterator_destroy(git);
err_free_graph:
	matrix_graph_destroy(graph);
	return (rc ? rc : -1);
}
