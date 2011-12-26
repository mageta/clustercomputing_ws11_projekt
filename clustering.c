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

//int main(int argc, char** argv)
int main(int argc, char *argv[]) {
	int rank;	// rank of the process
	int p;		// number of process
	MPI_Status status;
	MPI_Datatype mpi_matrix_type;

	// some mpi crap
  	MPI_Init(&argc, &argv);
  	MPI_Comm_size(MPI_COMM_WORLD, &p);
  	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	printf("process %d of %d reports for duty!\n", rank, p);

	int i,j;
	matrix_type *matrix;

	int* matrix_to_transfer;
	matrix_to_transfer = (int*)calloc( N*M, sizeof(int));

	// read matrix from file
	if(read_input_file(&matrix, argv[1])) {
		fprintf(stderr, "\n%s\n", usage());
		return -1;
	}

	// print matrix to stdout
	fprintf(stdout, "test_matrix:\n");
	for (i = 0; i < matrix->m; i++) {
		for (j = 0; j < (matrix->n - 1); j++) {
			fprintf(stdout, "%hd, ", *((unsigned short int *)
						matrix_get(matrix, i, j)));
		}
		fprintf(stdout, "%hd\n", *((unsigned short int *)
					matrix_get(matrix, i, j)));
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

	if (rank == 0) { // sender
		printf("%s\n", "sender is active, sending matrix ...");
		MPI_Barrier( MPI_COMM_WORLD);
		MPI_Send(matrix_to_transfer, 1, mpi_matrix_type, 1, 0, MPI_COMM_WORLD);
	} else {
		printf("%s%i%s\n", "receiver ", rank , " is active, receiving matrix ...");
		MPI_Barrier( MPI_COMM_WORLD);
		MPI_Recv(matrix_to_transfer, 1, mpi_matrix_type, 0, 0, MPI_COMM_WORLD, &status);
		printmat(matrix_to_transfer);
		
	}

	free(matrix_to_transfer);
	MPI_Type_free(&mpi_matrix_type);
	MPI_Finalize();

	return 0;
}

