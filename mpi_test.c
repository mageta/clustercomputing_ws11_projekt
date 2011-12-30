#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "mpi.h"
#include "list.h"
#include "queue.h"
#include "vector.h"
#include "stack.h"
#include "matrix.h"

#define COMM_DIMS 2

#define MPI_TAG_COMLIST_SIZE	0
#define MPI_TAG_COMLIST		1

struct component {
	int proc_rank;
	unsigned int example_coords[COMM_DIMS];
	unsigned int size;
	unsigned short int component_id;
} __attribute__((__packed__));

static MPI_Datatype MPI_component_type;

static void register_mpi_component_type()
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
		MPI_UNSIGNED_SHORT,
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

struct component_list {
	/* containes 'struct component' */
	vector_type *components;
};

struct processor_data {
	int rank;
	int coords[COMM_DIMS];

	/* contains 'struct component_list' */
	list_type *component_lists;
};

int mpi_working_function(MPI_Comm *comm, int proc_count, int *dims,
		struct processor_data *pdata);

int main(int argc, char **argv)
{
	/*
	 * MPI-Variablen:
	 * rank	- Rang des Prozesses
	 * p	- Anzahl Prozesse
	 */
	int rank;
	int p;

	int rc = 0, i;

	int dims[COMM_DIMS], periods[COMM_DIMS];

	struct processor_data pdata;

	MPI_Comm grid_comm;

	srand(time(NULL));

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &p);

	/* define user-datatype */

	register_mpi_component_type();

	/* create topo */

	rc = list_create(&pdata.component_lists, sizeof(struct component_list));
	if(rc) goto err_out;

	for(i = 0; i < COMM_DIMS; i++) {
		dims[i] = 0;
		periods[i] = 0;
	}

	MPI_Dims_create(p, COMM_DIMS, dims);

	MPI_Cart_create(MPI_COMM_WORLD, COMM_DIMS, dims, periods, 1, &grid_comm);
	MPI_Comm_rank(grid_comm, &pdata.rank);
	MPI_Cart_coords(grid_comm, pdata.rank, COMM_DIMS, pdata.coords);


	/* call main working function */

	rc = mpi_working_function(&grid_comm, p, dims, &pdata);

	list_destroy(pdata.component_lists);

	MPI_Type_free(&MPI_component_type);

	MPI_Finalize();
	return rc;
err_out:
	MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
	return rc;
}

static unsigned int urand()
{
	int urandom = open("/dev/urandom", O_RDONLY);
	unsigned int rc;

	read(urandom, &rc, sizeof(rc));

	close(urandom);

	return rc;
}

int mpi_working_function(MPI_Comm *comm, int proc_count, int *dims,
		struct processor_data *pdata)
{
	int i, j, k, rc, found;
	int last = 1, cid = 0;
	int comm_coords[COMM_DIMS], comm_rank, comm_len = 0;
	size_t comm_size = 0;

	MPI_Status status;

	/* local components */
	struct component_list local_comp_list, comm_comp_list, *complistp;
	struct component local_comp, *compp;

	rc = vector_create(&local_comp_list.components, 0, sizeof(struct component));
	if(rc) goto err_out;

	do {
		local_comp.proc_rank = pdata->rank;
		local_comp.component_id = ++cid;
		local_comp.example_coords[0] = urand() % 50 + (pdata->coords[0] * 50);
		local_comp.example_coords[1] = urand() % 50 + (pdata->coords[1] * 50);
		local_comp.size = urand() % 10 + 1;

		rc = vector_add_value(local_comp_list.components, &local_comp);
		if(rc) goto err_out;
	} while((urand() % 100) < 50);

	rc = list_append(pdata->component_lists, &local_comp_list);
	if(rc) goto err_out;

	/*
	 * try to recive something from
	 *
	 * first above, second left neighbour
	 */
	for(i = 0; i < COMM_DIMS; i++) {
		for(j = 0; j < COMM_DIMS; j++) {
			comm_coords[(i + j) % COMM_DIMS] =
				pdata->coords[(i + j) % COMM_DIMS] - (!j ? 1 : 0);
		}

		if(comm_coords[i] < 0)
			continue;

		MPI_Cart_rank(*comm, comm_coords, &comm_rank);

		MPI_Recv(&comm_size, 1, MPI_UNSIGNED, comm_rank,
				MPI_TAG_COMLIST_SIZE, *comm, &status);

		for(k = 0; k < comm_size; k++) {
			MPI_Probe(comm_rank, MPI_TAG_COMLIST, *comm, &status);
			MPI_Get_count(&status, MPI_component_type, &comm_len);

			rc = vector_create(&comm_comp_list.components, comm_len,
					sizeof(struct component));
			if(rc) goto err_out;

			MPI_Recv(comm_comp_list.components->values,
					comm_len, MPI_component_type, comm_rank,
					MPI_TAG_COMLIST, *comm, &status);

			/* set the count of wirtten elements in the vector */
			comm_comp_list.components->elements = comm_len;

			/*
			 * look whether this component_list is already in the list
			 * of component_lists that we know
			 */
			found = 0;
			for(j = 0; j < pdata->component_lists->elements; j++) {
				int rank1, rank2;

				compp = (struct component *) vector_get_value(
							comm_comp_list.components, 0);
				rank1 = compp->proc_rank;

				complistp = (struct component_list *)
					list_element(pdata->component_lists, j);
				compp = (struct component *) vector_get_value(
						complistp->components, 0);
				rank2 = compp->proc_rank;

				if(rank1 == rank2) {
					found = 1;
					break;
				}
			}

			if(!found) {
				rc = list_append(pdata->component_lists,
						&comm_comp_list);
				if(rc) goto err_out;
			} else {
				vector_destroy(comm_comp_list.components);
			}
		}
	}

	/*
	 * try to send to your right and bottom neighbour
	 *
	 * first down, second right
	 */
	for(i = 0; i < COMM_DIMS; i++) {
		for(j = 0; j < COMM_DIMS; j++) {
			comm_coords[(i + j) % COMM_DIMS] =
				pdata->coords[(i + j) % COMM_DIMS] + (!j ? 1 : 0);
		}

		if(comm_coords[i] >= dims[i])
			continue;

		MPI_Cart_rank(*comm, comm_coords, &comm_rank);

		/* send count of component_lists */
		MPI_Send(&pdata->component_lists->elements, 1, MPI_UNSIGNED,
				comm_rank, MPI_TAG_COMLIST_SIZE, *comm);

		for(k = 0; k < pdata->component_lists->elements; k++) {
			list_get(pdata->component_lists, k, &comm_comp_list);

			MPI_Ssend(comm_comp_list.components->values,
					comm_comp_list.components->elements,
					MPI_component_type, comm_rank,
					MPI_TAG_COMLIST, *comm);
		}

		last = 0;
	}

	if(last) {
		fprintf(stdout, "comm finished\nlists on last node:\n");

		for(int i = 0; i < pdata->component_lists->elements; i++) {
			complistp = (struct component_list *)
				list_element(pdata->component_lists, i);
			compp = (struct component *) vector_get_value(
					complistp->components, 0);

			fprintf(stdout, "list from rank %d:\n", compp->proc_rank);

			for(int j = 0; j < complistp->components->elements; j++) {
				compp = (struct component *) vector_get_value(
						complistp->components, j);

				fprintf(stdout, "example_coords: %d, %d; "
						"size: %d; component_id: %hd\n",
						compp->example_coords[0],
						compp->example_coords[1],
						compp->size, compp->component_id);
			}
		}
	}

	k = pdata->component_lists->elements;
	for(i = 0; i < k; i++) {
		list_get_head(pdata->component_lists, &local_comp_list);
		list_remove_head(pdata->component_lists);

		vector_destroy(local_comp_list.components);
	}

	return 0;
err_out:
	MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
	fprintf(stderr, "some operation failed with rc.. %d, '%s'\n",
			rc, strerror(rc));
	return rc;
}
