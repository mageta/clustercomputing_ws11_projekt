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

#define MPI_TAG_BASE		1
#define MPI_TAG_MATRIX_DIMS	MPI_TAG_BASE + 1
#define MPI_TAG_MATRIX		MPI_TAG_MATRIX_DIMS + 1
#define MPI_TAG_BORDER		MPI_TAG_MATRIX + 1
#define MPI_TAG_COMPONENTS	MPI_TAG_BORDER + 1
#define MPI_TAG_COMLIST		MPI_TAG_COMPONENTS + 1

MPI_Datatype MPI_component_type;

static void	register_mpi_component_type	();

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

void register_mpi_component_type()
{
	struct component c;
	int count = 5;
	int blocklens[] = {1, COMM_DIMS, 1, 1, 1};
	MPI_Aint displ[] = {
		0,
		(long) &c.example_coords[0] - (long) &c,
		(long) &c.size - (long) &c,
		(long) &c.component_id - (long) &c,
		sizeof(struct component)
	};
	MPI_Datatype oldt[] = {
		MPI_INT,
		MPI_UNSIGNED,
		MPI_UNSIGNED,
		MPI_UNSIGNED,
		MPI_UB
	};
	/*
	 * this whole MPI_Type-thingy is realy fragile,
	 * you should double-check if the resulting MPI_Datatype has the same length
	 * as the c-datatype. MPI_Type_extent (!).
	 * To get more confidenz in this, order the fields in the
	 * struct according to the common padding-rules (aligend to the biggest type)
	 * and finish the MPI_Dt with MPI_UB, like I did.
	 */

	MPI_Type_struct(count, blocklens, displ, oldt, &MPI_component_type);
	MPI_Type_commit(&MPI_component_type);
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
	// int sum = 0;

	MPI_Status status;
	MPI_Datatype mpi_matrix_type;
	MPI_Datatype mpi_matrix_dims_type;
	MPI_Datatype mpi_border_type;
	// MPI_Datatype mpi_components_type;

	unsigned int matrix_dims[2];

	// some mpi crap
  	MPI_Init(&argc, &argv);
  	MPI_Comm_size(MPI_COMM_WORLD, &p);
  	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	// printf("process %d of %d reports for duty!\n", rank, p);

	int i;
	matrix_type *input_matrix;

	// find_components
	struct component_list *components, recv_components;
	vector_type *borders;
	matrix_type *border;
	unsigned int cid;
	unsigned int mid;
	unsigned int mid_prev = -1;
	unsigned int mid_next = -1;
	int comm_len = 0;

	recv_components.components = NULL;

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
	int to = from + number_of_ranges; // end of the range

	// if N divided by p equals an odd number, the range of the last node
	// will be expanded to N
	if (input_matrix->n % p != 0) {
		last_range = input_matrix->n - (number_of_ranges * p);
	}
	if ( rank == p - 1) { // last node
		to = to + last_range;
	}

	matrix_type * working_matrix;

	rc = matrix_create(&working_matrix, input_matrix->m, to - from, sizeof(TYPE_WOKRING_MATRIX));
	if(rc)
		goto err_out;

	/* copy the selcted columns into the working matrix */
	for(int row = 0, i = 0; row < input_matrix->m; row++, i++)
		for(int col = from, j = 0; col < (to); col++, j++)
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
	// the right border is the one to be sent to the next node
	border = vector_get_value(borders, BORDER_RIGHT);
	/**
	 * communication in-between the nodes
	 */

	/* new datatype for border, has just 1 row */
	MPI_Type_vector(border->m, border->n, border->n, MPI_UNSIGNED_CHAR, &mpi_border_type);
	MPI_Type_commit(&mpi_border_type);

	// datatype for components
	register_mpi_component_type();

	// all nodes except the first one are receiving the right border from the left neighbour
	if (rank > 0) {
		source = rank - 1;

		MPI_Probe(source, MPI_TAG_COMLIST, MPI_COMM_WORLD, &status);
		rc = MPI_Get_count(&status, MPI_component_type, &comm_len);
		if(rc) {
			fprintf(stderr, "ERR in %s:%d:%s()\n", __FILE__, __LINE__, __func__);
			goto err_out;
		}

		printf("elements: %i\n", comm_len);
		rc = vector_create(&recv_components.components, comm_len, sizeof(struct component));
		if(rc) {
			fprintf(stderr, "ERR in %s:%d:%s()\n", __FILE__, __LINE__, __func__);
			goto err_out;
		}

		rc = MPI_Recv(recv_components.components->values, comm_len, MPI_component_type, source, MPI_TAG_COMLIST, MPI_COMM_WORLD, &status);
		if(rc) {
			fprintf(stderr, "ERR in %s:%d:%s()\n", __FILE__, __LINE__, __func__);
			goto err_out;
		}

		printf("rank=%i: components (%i) transfer: %i <- %i\n", rank,components->components->elements,rank,source);

		rc = MPI_Recv(border->matrix, 1, mpi_border_type, source, MPI_TAG_BORDER, MPI_COMM_WORLD, &status);
		if(rc) {
			fprintf(stderr, "ERR in %s:%d:%s()\n", __FILE__, __LINE__, __func__);
			goto err_out;
		}

		printf("rank=%i: border transfer: %i <- %i\n",rank, rank, source);
		printf("rank=%i: border matches (current rank=%i):\n\nNr: recv- own value",rank,rank);
		for(i = 0; i < border->m; i++) {
			cid = *((unsigned int *) matrix_get(border,i, 0));
			mid = *((unsigned char *) matrix_get(working_matrix, i, 0));
			if(!cid)
				fprintf(stdout, "\n %i:  0  - %2d",i, mid);
			else { // if cid = 1 ...
				fprintf(stdout, "\n %i: %2d  - %2d",i,cid, mid);

				/** if both id's are 1
				 Example:     0 0
				            ->1 1
				              0 0
				 */
				if ((mid != 0)) { // if both id's are 1
					printf(" x");
				}

				// now the diagonal connections:

				/** if the upper right mid is 1
				 Example:     0 1
				            ->1 0
				              0 0
				 */
				if(i > 0) {
					mid_prev = *((unsigned char *) matrix_get(working_matrix, i - 1, 0));
					if (cid !=0 && mid_prev > 0) {
						printf(" T");
					}
				}

				/** if the bottom right mid is 1
				 Example:     0 0
				            ->1 0
				              0 1
				 */
				if(i < (border->m) - 1) {
					mid_next = *((unsigned char *) matrix_get(working_matrix, i + 1, 0));
					if (mid_next > 0) {
						printf(" V");
					}
				}
			}
		}
		printf("\n");
		// print_workingmat(working_matrix);
	}

	// all nodes except the last one send their right border to the right neighbour
	if (rank < p - 1) {
		dest = rank + 1;
		rc = MPI_Send(components->components->values, components->components->elements,
				MPI_component_type, dest, MPI_TAG_COMLIST, MPI_COMM_WORLD);
		if(rc) {
			fprintf(stderr, "ERR in %s:%d:%s()\n", __FILE__, __LINE__, __func__);
			goto err_out;
		}
		printf("rank=%i: components (%i) transfer: %i -> %i\n",rank,components->components->elements, rank,dest);
		MPI_Send(border->matrix, 1, mpi_border_type, dest, MPI_TAG_BORDER, MPI_COMM_WORLD);
		printf("rank=%i: border transfer: %i -> %i\n",rank, rank, dest );
	}


	if(rc) {
		fprintf(stderr, "ERR in %s:%d:%s()\n", __FILE__, __LINE__, __func__);
	}

	// borders_destroy(borders);
	component_list_destroy(components);
	component_list_destroy(recv_components.components);
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
