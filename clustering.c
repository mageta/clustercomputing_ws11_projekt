#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>

#include "mpi.h"
#include "matrix.h"
#include "queue.h"
#include "vector.h"
#include "components.h"

#include <asm/errno.h>

#define TYPE_INPUT_MATRIX unsigned char
#define TYPE_WOKRING_MATRIX TYPE_INPUT_MATRIX

#define MPI_TAG_BASE		0
#define MPI_TAG_MATRIX_DIMS	MPI_TAG_BASE + 1
#define MPI_TAG_MATRIX		MPI_TAG_MATRIX_DIMS + 1
#define MPI_TAG_BORDER		MPI_TAG_MATRIX + 1

static char * usage() {
	static char text[256];
	int written;
	char *textp = text;

	written = sprintf(textp, "usage: connectivity <file>");
	textp += written;

	return text;
}

void print_inputmat(matrix_type *mat) {
	TYPE_INPUT_MATRIX value;
	for(int row = 0; row < mat->m; row++)
	{
		for(int col = 0; col < mat->n; col++)
		{
			value = *((TYPE_INPUT_MATRIX *) matrix_get(mat, row, col));
			printf("%hhd ", value);
		}
		printf("\n");
	}
}

void print_workingmat(matrix_type *mat) {
	TYPE_WOKRING_MATRIX value;
	for(int row = 0; row < mat->m; row++)
	{
		for(int col = 0; col < mat->n; col++)
		{
			value = *((TYPE_WOKRING_MATRIX *) matrix_get(mat, row, col));
			printf("%hhd ", value);
		}
		printf("\n");
	}
}

/*
 * reads the given file and tries to extract a matrix from it
 *
 * the type of each field in m will be TYPE_INPUT_MATRIX (unsigned char),
 * if successful
 */
static int read_input_file(matrix_type **m, char *file_name){
	int i, j, rc;
	unsigned int height, width;
	long int read_value;
	size_t line_length;
	TYPE_INPUT_MATRIX write_value;
	char *line = NULL, *endp, *strp;
	FILE *input = NULL;
	matrix_type *matrix;
	queue_type *lines;

	if(queue_create(&lines, sizeof(line))) {
		fprintf(stderr, "Not enough memory.\n");
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

	//fprintf(stdout, "height: %d; width: %d\n", height, width);

	rc = matrix_create(&matrix, height, width, sizeof(write_value));
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

int main(int argc, char *argv[]) {
	int rc;
	int rank;	// rank of the process
	int p;		// number of process
	int source;
	int dest;
	int tag;
	int sum = 0;

	MPI_Status status;
	MPI_Datatype mpi_matrix_type;
	MPI_Datatype mpi_matrix_dims_type;
	MPI_Datatype mpi_border_type;

	int column;
	unsigned int matrix_dims[2];

	// some mpi crap
  	MPI_Init(&argc, &argv);
  	MPI_Comm_size(MPI_COMM_WORLD, &p);
  	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	// printf("process %d of %d reports for duty!\n", rank, p);

	int i,j;
	matrix_type *input_matrix;

	// find_components
	struct component_list *components;
	vector_type *borders;
	matrix_type *border;
	unsigned int *cid;



	if(p < 2) {
		printf("you should definitely start that program with more than 1 processes\n");
		MPI_Finalize();
		return 1;
	}

	MPI_Type_contiguous(2, MPI_UNSIGNED, &mpi_matrix_dims_type);
	MPI_Type_commit(&mpi_matrix_dims_type);

	/**
	 *	sending the matrix to every node
	 */

	if (rank == 0) { // sender/master
		// read input_matrix from file, only root has the file
		if(read_input_file(&input_matrix, argv[1])) {
			fprintf(stderr, "\n%s\n", usage());
			return -1;
		}

		matrix_dims[0] = input_matrix->m;
		matrix_dims[1] = input_matrix->n;

		// the new datatype for the matrix
		/* the input_matrix has the type (unsigned char) -> see TYPE_INPUT_MATRIX */
		MPI_Type_vector(input_matrix->m, input_matrix->n, input_matrix->n, MPI_UNSIGNED_CHAR, &mpi_matrix_type);
		MPI_Type_commit(&mpi_matrix_type);

		tag = 0;
		for( dest = 1; dest < p; dest++) {
			printf("rank=%i: Sending matrix to rank=%i\n", rank, dest);
			MPI_Send(matrix_dims, 1, mpi_matrix_dims_type, dest, MPI_TAG_MATRIX_DIMS, MPI_COMM_WORLD);
			MPI_Send(input_matrix->matrix, 1, mpi_matrix_type, dest, MPI_TAG_MATRIX, MPI_COMM_WORLD);
		}
	} else { // anything but master
		tag = 0;
		source = 0; // from master

		/* first: get the dimensions of the input-matrix */
		MPI_Recv(matrix_dims, 1, mpi_matrix_dims_type, source, MPI_TAG_MATRIX_DIMS, MPI_COMM_WORLD, &status);

		rc = matrix_create(&input_matrix, matrix_dims[0], matrix_dims[1], sizeof(TYPE_INPUT_MATRIX));
		if(rc) /* most likely: out of memory */
			goto err_out;

		/* commit the received matrix dimension into the mpi_matrix_type */
		MPI_Type_vector(input_matrix->m, input_matrix->n, input_matrix->n, MPI_UNSIGNED_CHAR, &mpi_matrix_type);
		MPI_Type_commit(&mpi_matrix_type);

		/* second: get the actual matrix */
		MPI_Recv(input_matrix->matrix, 1, mpi_matrix_type, source, MPI_TAG_MATRIX, MPI_COMM_WORLD, &status);
		printf("rank=%i: matrix received\n", rank);
	}

	// printf("input_matrix: \n");
	// print_inputmat(input_matrix);

	/**
	 *	calculation
	 */
	int number_of_ranges = input_matrix->n / p; // in how many ranges the matrix will be devided
	int last_range = 0;
	int from = rank * number_of_ranges; // beginning of the range
	int to = from + p; // end of the range

	// if N divided by p equals an odd number, the range of the last node
	// will be expanded to N
	if (input_matrix->n % p != 0) {
		last_range = input_matrix->n - (number_of_ranges * p);
	}
	if ( rank == p - 1) { // last node
		to = to + last_range;
	}

	matrix_type * working_matrix;

	rc = matrix_create(&working_matrix, input_matrix->m, to - from + 1, sizeof(TYPE_WOKRING_MATRIX));
	if(rc)
		goto err_out;

	/* copy the selcted columns into the working matrix */
	for(int row = 0, i = 0; row < input_matrix->m; row++, i++)
		for(int col = from, j = 0; col < (to + 1); col++, j++)
			matrix_set(working_matrix, i, j, matrix_get(input_matrix, row, col));

	// printf("rank=%i is now printing his columns from %i tp %i\n", rank, from, to);
	// print_workingmat(working_matrix);

	/**
	 * finding components
	 */
	rc = find_components(working_matrix, &components, &borders);
	if(rc) {
		fprintf(stderr, "find_components failed.. %s\n", strerror(rc));
		goto err_out;
	}
	printf("number of components: %i\n",components->components->elements);

	// the right border is the one to be sent to the next node
	border = vector_get_value(borders, BORDER_RIGHT);
	/**
	 * communication in-between the nodes
	 */

	/* new datatype for border, has just 1 row */
	MPI_Type_vector(border->m, border->n, border->n, MPI_UNSIGNED_CHAR, &mpi_border_type);
	MPI_Type_commit(&mpi_border_type);

	// all nodes except the last one send their reight border to the right neighbour
	if (rank < p - 1) {
		column = rank;
		dest = rank + 1;
		MPI_Send(&border, 1, mpi_border_type, dest, MPI_TAG_BORDER, MPI_COMM_WORLD);
		printf("border transfer: %i -> %i\n", rank, dest );
	}

	// all nodes except the first one are receiving the right border from the left neighbour
	if (rank > 0) {
		source = rank - 1;
		MPI_Recv(&border, 1, mpi_border_type, source, MPI_TAG_BORDER, MPI_COMM_WORLD, &status);
		printf("border transfer: %i <- %i\n\n", rank, source);
		printf("%2d\n", matrix_get(&border, 1, 1));
		// ? ist das so richtig? ich moechte hier an besten die uebertragene border ausgeben
	}





	// borders_destroy(borders);
	component_list_destroy(components);
	matrix_destroy(working_matrix);
	matrix_destroy(input_matrix);
	MPI_Type_free(&mpi_matrix_type);
	MPI_Type_free(&mpi_matrix_dims_type);
	MPI_Finalize();
	return 0;
err_out:
	MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
	return rc;
}
