#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// #include <malloc.h>
#include "mpi.h"
#include <errno.h>
#include <limits.h>

#include "matrix.h"
#include "queue.h"

#include <asm/errno.h>


// 19 x 8 matrix
const int N = 19;
const int M = 8;

static char * usage() {
	static char text[256];	
	int written;
	char *textp = text;

	written = sprintf(textp, "usage: connectivity <file>");
	textp += written;

	return text;
}

void printmat(int* matrix) {
	for(int row = 0; row < M; row++)
	{
		for(int col = 0; col < N; col++)
		{
			printf("%d ", matrix[N*row+col]);
		}
		printf("\n");
	}
}

void printcolumns(int* matrix, int newN) {
	for(int row = 0; row < M; row++)
	{
		for(int col = 0; col < newN; col++)
		{
			printf("%d ", matrix[newN*row+col]);
		}
		printf("\n");
	}
}

int* extract_col(int* matrix, int from, int to) {
	int* extracted_columns;
	int size = to - from;
	int i = 0;
	extracted_columns = (int*)calloc( size*M, sizeof(int));
	for(int row = 0; row < M; row++)
	{
		for(int col = from; col < to; col++)
		{
			extracted_columns[i] = matrix[N*row+col];
			i++;
		}
	}
	return extracted_columns;
}

static int read_input_file(matrix_type **m, char *file_name){
	int i, j, rc;
	unsigned int height, width;
	long int read_value;
	size_t line_length;
	unsigned short int write_value;
	char *line = NULL, *endp, *strp;
	FILE *input = NULL;
	matrix_type *matrix;
	queue_type *lines;

	if(queue_create(&lines, 0, sizeof(line))) {
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

int main(int argc, char *argv[]) {
	int rank;	// rank of the process
	int p;		// number of process
	int source; 
	int dest;
	int tag;
	int sum = 0;
	MPI_Status status;
	MPI_Datatype mpi_matrix_type;
	int column;


	// some mpi crap
  	MPI_Init(&argc, &argv);
  	MPI_Comm_size(MPI_COMM_WORLD, &p);
  	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	// printf("process %d of %d reports for duty!\n", rank, p);

	int i,j;
	matrix_type *matrix;

	int* matrix_to_transfer;
	matrix_to_transfer = (int*)calloc( N*M, sizeof(int));

	// read matrix from file
	if(read_input_file(&matrix, argv[1])) {
		fprintf(stderr, "\n%s\n", usage());
		return -1;
	}

	for (i = 0; i < matrix->m; i++) {
		for (j = 0; j < (matrix->n ); j++) {
			matrix_to_transfer[N*i+j] = *(((unsigned short int *)matrix_get(matrix, i, j)));
		}
		matrix_to_transfer[N*i+j] = *(((unsigned short int *)matrix_get(matrix, i, j)));
	}

	// the new datatype for the matrix
	MPI_Type_vector(M, N, N, MPI_INT, &mpi_matrix_type);
	MPI_Type_commit(&mpi_matrix_type);

	if(p < 2) {
		printf("you should definitely start that program with more than 1 processes\n");
		MPI_Finalize();
		return 1;
	}


	/**
	 *	sending the matrix to every node
	 */

	if (rank == 0) { // sender/master
		tag = 0;
		for( dest = 1; dest < p; dest++) {
			printf("rank=%i: Sending matrix to rank=%i\n", rank, dest);
			MPI_Send(matrix_to_transfer, 1, mpi_matrix_type, dest, tag, MPI_COMM_WORLD);
		}	
	} else { // anything but master
		tag = 0;
		source = 0; // from master
		MPI_Recv(matrix_to_transfer, 1, mpi_matrix_type, source, tag, MPI_COMM_WORLD, &status);
		printf("rank=%i: matrix received\n", rank);
	}


	/**
	 *	basic communication in-between the nodes
	 */

	// all nodes except the last one send their column to the next one 
	if (rank < p - 1) { 
		column = rank;
		dest = rank + 1;
		MPI_Send(&column, 1, MPI_INT, dest, 5, MPI_COMM_WORLD);
		printf("rank=%i: column has been sent to rank=%i\n", rank, dest );
	}
	
	// all nodes except the first one are receiving the column of the previous one
	if (rank > 0) {
		source = rank - 1;
		MPI_Recv(&column, 1, MPI_INT, source, 5, MPI_COMM_WORLD, &status);
		printf("rank=%i: column has been received from rank=%i\n", rank, source);
	}

	/**
	 *	calculation	
	 */
	int number_of_ranges = N / p; // in how many ranges the matrix will be devided
	int last_range = 0;
	int from = rank * number_of_ranges; // beginning of the range 
	int to = from + p; // end of the range

	// if N divided by p equals an odd number, the range of the last node
	// will be expanded to N	
	if (N % p != 0) {
		last_range = N - (number_of_ranges * p);
	}
	if ( rank == p - 1) { // last node
		to = to + last_range;		
	}

	int* cols = extract_col(matrix_to_transfer, from, to);
	printf("rank=%i is now printing his columns from %i tp %i\n",rank, from, to);
	printcolumns(cols, to-from);

		
	matrix_destroy(matrix);
	free(matrix_to_transfer);
	MPI_Type_free(&mpi_matrix_type);
	MPI_Finalize();
	return 0;
}

