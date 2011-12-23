#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// #include <malloc.h>
#include "mpi.h"
#include <errno.h>
#include <limits.h>

#include "helper/matrix.h"
#include "helper/queue.h"

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

static int read_input_file(matrix_type **m, char *file_name)
{
	int i,j;
	unsigned int height, width;
	long int read_value;
	// unsigned int line_length;
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
	strp = line;
	width = 0;

	strp = strtok(line, ", \n");
	while(strp) {
		read_value = strtol(strp, &endp, 10);

		if((errno == ERANGE) || (strp == endp)) {
			fprintf(stderr, "file '%s' contains invalid input.\n",
					file_name);
			goto err_free_queue;
		}

		width++;

		strp = strtok(NULL, ", \n");
	}

	height = queue_size(lines);

	fprintf(stdout, "height: %d; width: %d\n", height, width);

	if(matrix_create(&matrix, height, width, sizeof(unsigned short int))) {
		fprintf(stderr, "Not enougth memory.\n");
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
				fprintf(stderr, "file '%s' contains invalid input.\n",
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

int main(int argc, char** argv)
{
	int rank;	// rank of the process
	int p;		// number of process
	// MPI_Status status;
	MPI_Datatype mpi_matrix_type;

	int i,j;
	matrix_type *matrix;

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

	// some mpi crap
	if (MPI_Init(&argc,&argv)!=MPI_SUCCESS) {
        fprintf(stderr, "MPI_Init failed.");
        return -1;
    }
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &p);

	// the new datatype for the matrix
	MPI_Type_vector(M, N, N, MPI_INT, &mpi_matrix_type);
	MPI_Type_commit(&mpi_matrix_type);

	if(p<=1) {
		printf("you should defitively start that programm with more than 1 processes\n");
		MPI_Finalize();
		return 1;
	}

	return 1;
}

