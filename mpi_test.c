#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#include "mpi.h"
#include "list.h"
#include "queue.h"
#include "vector.h"
#include "stack.h"
#include "matrix.h"

#define COMM_DIMS 2

int mpi_working_function(MPI_Comm *comm, int rank, int proc_count,
		int *coords, int *dims);

int main(int argc, char **argv)
{
	/*
	 * MPI-Variablen:
	 * rank	- Rang des Prozesses
	 * p	- Anzahl Prozesse
	 */
	int rank, grid_rank;
	int p;

	int rc = 0, i;

	int ndims = COMM_DIMS;
	int dims[COMM_DIMS], coords[COMM_DIMS], periods[COMM_DIMS];

	MPI_Comm grid_comm;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &p);

	for(i = 0; i < ndims; i++) {
		dims[i] = 0;
		periods[i] = 0;
	}

	MPI_Dims_create(p, ndims, dims);

	MPI_Cart_create(MPI_COMM_WORLD, ndims, dims, periods, 1, &grid_comm);
	MPI_Comm_rank(grid_comm, &grid_rank);
	MPI_Cart_coords(grid_comm, grid_rank, ndims, coords);

	fprintf(stdout, "hi, here is %d of %d. In grid_comm I'm %d and have the coordinates (%d, %d)\n", rank, p, grid_rank, coords[0], coords[1]);

	rc = mpi_working_function(&grid_comm, grid_rank, p, coords, dims);

	MPI_Finalize();
	return rc;
}

static int refresh_matrix_with(matrix_type *dest, matrix_type *src)
{
	int i, j;

	if(!dest || !src || (dest->m != src->m) || (dest->n != src->n))
		return -1;

	for(i = 0; i < dest->m; i++) {
		for(j = 0; j < dest->n; j++) {
			if(!(*((int *) matrix_get(dest, i, j))))
				matrix_set(dest, i, j, ((int *)
							matrix_get(src, i, j)));
		}
	}

	return 0;
}

int mpi_working_function(MPI_Comm *comm, int rank, int proc_count,
		int *coords, int *dims)
{
	int last = 1, i, j;
	int comm_coords[COMM_DIMS], comm_rank;
	matrix_type *matrix, *comm_matrix;

	MPI_Status status;

	matrix_create(&matrix, dims[0], dims[1], sizeof(rank));
	matrix_init(matrix, 0);
	matrix_copy(&comm_matrix, matrix);

	matrix_set(matrix, coords[0], coords[1], &rank);

	/* erst versuchen etwas zu empfangen */

	/* von links */
	if((coords[1] - 1) >= 0) {
		comm_coords[0] = coords[0];
		comm_coords[1] = coords[1] - 1;
		MPI_Cart_rank(*comm, comm_coords, &comm_rank);

		MPI_Recv(comm_matrix->matrix, matrix_size(comm_matrix),
				MPI_INT, comm_rank, 0, *comm, &status);
		refresh_matrix_with(matrix, comm_matrix);
	}

	/* von oben */
	if((coords[0] - 1) >= 0) {
		comm_coords[0] = coords[0] - 1;
		comm_coords[1] = coords[1];
		MPI_Cart_rank(*comm, comm_coords, &comm_rank);

		MPI_Recv(comm_matrix->matrix, matrix_size(comm_matrix),
				MPI_INT, comm_rank, 1, *comm, &status);
		refresh_matrix_with(matrix, comm_matrix);
	}

	/*
	 * dann versuchen etwas zu senden
	 *
	 * das Ergebnis liegt dann beim letzten, dem einzigen der an niemanden
	 * etwas sendet.
	 */

	/* nach links */
	if((coords[1] + 1) < dims[1]) {
		comm_coords[0] = coords[0];
		comm_coords[1] = coords[1] + 1;
		MPI_Cart_rank(*comm, comm_coords, &comm_rank);

		MPI_Send(matrix->matrix, matrix_size(matrix),
				MPI_INT, comm_rank, 0, *comm);

		last = 0;
	}

	/* nach unten */
	if((coords[0] + 1) < dims[0]) {
		comm_coords[0] = coords[0] + 1;
		comm_coords[1] = coords[1];
		MPI_Cart_rank(*comm, comm_coords, &comm_rank);

		MPI_Send(matrix->matrix, matrix_size(matrix),
				MPI_INT, comm_rank, 1, *comm);

		last = 0;
	}

	if(last) {
		fprintf(stdout, "communication finished.\n");
		for(i = 0; i < dims[0]; i++) {
			for(j = 0; j < (dims[1] - 1); j++) {
				fprintf(stdout, "%d, ", *((int *) matrix_get(
								matrix, i, j)));
			}
			fprintf(stdout, "%d\n", *((int *) matrix_get(
							matrix, i, j)));
		}
	}

	return 0;
}
