#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>

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
#include "components.h"

#define MPI_TAG_BASE		0
#define MPI_TAG_COMLIST_SIZE	MPI_TAG_BASE + 1
#define MPI_TAG_COMLIST		MPI_TAG_COMLIST_SIZE + 1
#define MPI_TAG_MAT_DIST	MPI_TAG_COMLIST + 1
#define MPI_TAG_BORDER		MPI_TAG_MAT_DIST + 1

static void debug_out(int id) {
	static int debug_nr = 0;
	fprintf(stderr, "%d: debug_point %d\n", id, debug_nr++);
}

static MPI_Datatype MPI_component_type;
static MPI_Datatype MPI_matrix_dims;

struct processor_data {
	/* MPI_COMM_WORLD */
	int rank;

	/*
	 * topology for the communication during the
	 * component-recognition-stage
	 *
	 * rank is the same, as in MPI_COMM_WORLD
	 */
	MPI_Comm topo;
	int topo_size;
	int coords[COMM_DIMS];

	/*
	 * contains 'struct component_list' see comment of transform_clist()
	 */
	list_type *component_lists;
	matrix_type *matrix;
	vector_type *borders;
};

struct matrix_dims {
	unsigned int m, n;
} __attribute__((__packed__));

/* registers the used MPI-types (see above) */
static void	register_mpi_component_type	();
static void	register_mpi_matrix_dims	();

/* prints usage message */
static char *	usage				(int argc, char ** argv);

/* functions containing mpi-stuff */
static int	mpi_working_function		(struct processor_data *pdata,
						 int *dims);
static int	mpi_distribute_matrix		(struct processor_data *pdata,
						 int *dims,
						 matrix_type *input_matrix);
static int	mpi_receive_matrix		(struct processor_data *pdata);

/* diverse helper */
static int	read_input_file			(matrix_type **m,
						char *file_name);
static int	transform_clist			(struct processor_data *pdata,
						 struct component_list *clist);
static void	print_component_lists		(list_type *component_lists);
static void	print_inputmatrix		(matrix_type *matrix);
static void	print_border			(matrix_type *border,
						 int dimension);

int main(int argc, char **argv)
{
	/*
	 * MPI-Variablen:
	 * p	- Anzahl Prozesse
	 */
	int p;

	int rc = 0, i;

	int dims[COMM_DIMS], periods[COMM_DIMS];

	struct processor_data pdata;
	struct component_list *clist;

	matrix_type *input_matrix;

	srand(time(NULL));

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &pdata.rank);
	MPI_Comm_size(MPI_COMM_WORLD, &p);

	/* define user-datatype */

	register_mpi_component_type();
	register_mpi_matrix_dims();

	/*
	 * create a topology for the communication during the component-
	 * recognition-stage of the algorithm
	 *
	 * m x n:
	 * 0 ...... n - 1
	 * .          .
	 * .          .
	 * .          .
	 * .          .
	 * m - 1 .. n - 1 / m -1
	 */

	rc = list_create(&pdata.component_lists,
			sizeof(struct component_list));
	if(rc) goto err_output;

	for(i = 0; i < COMM_DIMS; i++) {
		dims[i] = 0;
		periods[i] = 0;
	}
	/*
	 * the first version will only support a topology with m = 1
	 *
	 * so its more like:
	 * 0 ........ (n - 1 == processesor-count - 1)
	 */
	dims[0] = 1;

	MPI_Dims_create(p, COMM_DIMS, dims);

	MPI_Cart_create(MPI_COMM_WORLD, COMM_DIMS, dims, periods, 0,
			&pdata.topo);
	MPI_Cart_coords(pdata.topo, pdata.rank, COMM_DIMS, pdata.coords);
	MPI_Comm_size(pdata.topo, &pdata.topo_size);

	/* distribute / recive the input-matrix */

	input_matrix = NULL;
	pdata.matrix = NULL;
	if(pdata.rank == 0) {
		if(read_input_file(&input_matrix, argv[1])) {
			fprintf(stderr, "\n%s\n", usage(argc, argv));
			goto err_free_clists;
		}
		rc = mpi_distribute_matrix(&pdata, dims, input_matrix);
	} else {
		rc = mpi_receive_matrix(&pdata);
	}

	if(rc)
		goto err_free_matrix;

	/*
	 * the matrix-chunk is now available in pdata.matrix
	 * lets find the local components
	 */

	rc = find_components(pdata.matrix, &clist, &pdata.borders);
	if(rc)
		goto err_free_matrix;

	rc = transform_clist(&pdata, clist);
	if(rc)
		goto err_free_list;

	/*
	 * no longer needed as all components of it are part of the
	 * pdata->component_lists structure
	 */
	component_list_destroy(clist);
	clist = NULL;

	print_component_lists(pdata.component_lists);

	/* call main working function */

	rc = mpi_working_function(&pdata, dims);
	if(rc)
		goto err_free_list;

	/*
	 * clean up
	 */
	rc = 0;
err_free_list:
	borders_destroy(pdata.borders);
	component_list_destroy(clist);
err_free_matrix:
	matrix_destroy(pdata.matrix);
	MPI_Comm_free(&pdata.topo);
err_free_clists:
	list_destroy(pdata.component_lists);

	MPI_Type_free(&MPI_matrix_dims);
	MPI_Type_free(&MPI_component_type);
err_output:
	if(!rc) {
		MPI_Finalize();
	} else {
		fprintf(stderr, "%d: failed with.. %s\n", pdata.rank,
				strerror(rc));
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
	}
	if(input_matrix)
		matrix_destroy(input_matrix);

	return rc;
}

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

static void register_mpi_matrix_dims()
{
	MPI_Type_contiguous(2, MPI_UNSIGNED, &MPI_matrix_dims);
	MPI_Type_commit(&MPI_matrix_dims);
}

static char * usage(int argc, char ** argv)
{
	static char text[256];
	int written;
	char *textp = text;

	written = sprintf(textp, "usage: %s <file>", argv[0]);
	textp += written;

	return text;
}

static int read_input_file(matrix_type **m, char *file_name)
{
	int i, j, rc;
	unsigned int height, width;
	long int read_value;
	size_t line_length;
	unsigned char write_value;
	char *line = NULL, *endp, *strp;
	FILE *input = NULL;
	matrix_type *matrix;
	queue_type *lines;

	if(queue_create(&lines, sizeof(line))) {
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

//	fprintf(stdout, "height: %d; width: %d\n", height, width);

	rc = matrix_create(&matrix, height, width, sizeof(unsigned char));
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

/*
 * This function will transform a component_list returned by find_components
 * into the list-structure the rest of the algorithm expects.
 *
 * Essentialliy it takes each recognized component and creates a own
 * component_list for it and adds this list to the pdata's
 * component_lists-list. So, after this, each component has its own list.
 *
 * This is necessary so we can later recognize, if a component of an other
 * processesor was already added to a local component (to prevent duplicated
 * addition)
 *
 * To sum it up, after this call pdata->component_lists will contain a list of
 * (struct component_list) which in turn contains 1 component and thus is
 * representing a single component.
 *
 * pdata->component_lists (allocated pointer)
 *	-> struct component_list
 *		-> vector_type (allocated pointer)
 *			-> struct component
 */
static int transform_clist(struct processor_data *pdata,
		struct component_list *clist)
{
	int rc, i, max;
	struct component *comp_p;
	struct component_list comp_list;

	for(i = 0; i < clist->components->elements; i++) {
		comp_p = (struct component *)
				vector_get_value(clist->components, i);

		/* set the own rank, so we can later identify it */
		comp_p->proc_rank = pdata->rank;

		rc = vector_create(&comp_list.components, 1, sizeof(*comp_p));
		if(rc)
			goto err_free_lists;

		rc = vector_add_value(comp_list.components, comp_p);
		if(rc)
			goto err_free_lists;

		rc = list_append(pdata->component_lists, &comp_list);
		if(rc)
			goto err_free_lists;
	}

	return 0;
err_free_lists:
	max = pdata->component_lists->elements;
	for (i = 0; i < max; i++) {
		list_get_head(pdata->component_lists, &comp_list);
		list_remove_head(pdata->component_lists);

		vector_destroy(comp_list.components);
	}
	return rc;
}

/*
 * prints pdata->component_lists with each component_list and each
 * of their components
 */
static void print_component_lists(list_type *component_lists)
{
	int i, j;
	struct component *comp;
	struct component_list *clist;

	for (i = 0; i < component_lists->elements; i++) {
		clist = list_element(component_lists, i);

		fprintf(stdout, "clist %d:\n", i);

		for(j = 0; j < clist->components->elements; j++) {
			comp = (struct component *) vector_get_value(
						clist->components, j);

			fprintf(stdout, "comp %d, size %d, (%d, %d), "
					"proc_rank %d\n",
					comp->component_id, comp->size,
					comp->example_coords[0],
					comp->example_coords[1],
					comp->proc_rank);
		}
	}
}

/*
 * prints a matrix with (unsigned char) fields
 */
static void print_inputmatrix(matrix_type *matrix)
{
	int i, j;
	unsigned char val;

	for(i = 0; i < matrix->m; i++) {
		for(j = 0; j < (matrix->n - 1); j++) {
			val = *((unsigned char *) matrix_get(matrix, i, j));
			fprintf(stdout, (val? "%hhd, " : " , "), val);
		}
		val = *((unsigned char *) matrix_get(matrix, i, j));
		fprintf(stdout, (val? "%hhd\n" : "\n"), val);
	}
}

static void print_border(matrix_type *border, int dimension)
{
	int i;
	unsigned int val;

	for(i = 0; i < (dimension == 0 ? border->n : border->m) - 1; i++) {
		val = *((unsigned int *) matrix_get(border,
					(dimension == 0 ? 0 : i),
					(dimension == 0 ? i : 0)));
		fprintf(stdout, "%hhd ", val);
	}
	val = *((unsigned int *) matrix_get(border, (dimension == 0 ? 0 : i),
				(dimension == 0 ? i : 0)));
	fprintf(stdout, "%hhd\n", val);
}

static void __send_mat_chunk(matrix_type *input_matrix,
		unsigned int i, unsigned int j,
		unsigned int m, unsigned int n,
		MPI_Datatype *dt, MPI_Comm *comm)
{
	int coords[COMM_DIMS];
	int comm_rank;
	unsigned long displ;
	MPI_Request req;

	displ = (unsigned long) matrix_get(input_matrix, i * m, j * n)
		- (unsigned long) ((char *) input_matrix->matrix);

	coords[0] = i;
	coords[1] = j;
	MPI_Cart_rank(*comm, coords, &comm_rank);

	/* TODO: save the req somewhere to free it later */
	MPI_Isend(((char *) input_matrix->matrix) + displ, 1, *dt, comm_rank,
			MPI_TAG_MAT_DIST, *comm, &req);
	//MPI_Send(((char *) input_matrix->matrix) + displ, 1, *dt, comm_rank,
	//		MPI_TAG_MAT_DIST, *comm);
}

static int mpi_distribute_matrix (struct processor_data *pdata,
		int *topo_dims, matrix_type *input_matrix)
{
	int rc, i, j, disp;
	unsigned int dim_m, dim_n;
	unsigned int normal_n, remain_n;
	double n_per_proc;

	struct matrix_dims dims;

	matrix_type *dim_matrix, *disp_matrix, *count_matrix;
	MPI_Datatype normal_dt, remain_dt;

	/*
	 * calculate the dimension
	 */

	dim_m = input_matrix->m;
	dim_n = input_matrix->n;

	n_per_proc = dim_n / ((double) topo_dims[1]);
	if(n_per_proc < 1) {
		fprintf(stderr, "to many processors\n");
		return EPERM;
	}

	normal_n = n_per_proc;
	/*
	 * the last has to wait the longest time,
	 * so he can do a little more work
	 */
	remain_n = dim_n - normal_n * (topo_dims[1] - 1);

	/*
	 * prepare distribution of the dimension
	 */

	rc = matrix_create(&dim_matrix, topo_dims[0], topo_dims[1],
			sizeof(struct matrix_dims));
	if(rc)
		goto err_create_dim;

	rc = matrix_create(&disp_matrix, topo_dims[0], topo_dims[1],
			sizeof(int));
	if(rc)
		goto err_create_disp;

	rc = matrix_create(&count_matrix, topo_dims[0], topo_dims[1],
			sizeof(int));
	if(rc)
		goto err_create_count;

	matrix_init(count_matrix, 1);

	for (i = 0; i < dim_matrix->m; i++) {
		dims.m = dim_m;

		for (j = 0; j < (dim_matrix->n - 1); j++) {
			dims.n = normal_n;
			matrix_set(dim_matrix, i, j, &dims);

			disp = i * dim_matrix->n + j;
			matrix_set(disp_matrix, i, j, &disp);
		}

		dims.n = remain_n;
		matrix_set(dim_matrix, i, j, &dims);

		disp = i * dim_matrix->n + j;
		matrix_set(disp_matrix, i, j, &disp);
	}

	/*
	 * distribution dimensions
	 */

	MPI_Scatterv(dim_matrix->matrix, count_matrix->matrix,
			disp_matrix->matrix, MPI_matrix_dims,
			&dims, 1, MPI_matrix_dims, 0, pdata->topo);

	/* create the space for the local matrix */
	rc = matrix_create(&pdata->matrix, dims.m, dims.n,
			sizeof(unsigned char));
	if(rc)
		goto err_create_lmatrix;

	/*
	 * Prepare distribution of the matrix-chunks
	 *
	 * We can't use group-communication for this, because each chunk of the
	 * matrix starts at a other position. (we either can't use scatterv
	 * for this, see below)
	 *
	 * e.g.:
	 * Input is a matrix with dimensions 20x20 and there are 4 processors
	 * available.
	 *
	 * -> A chunk is 20x5 and has a stride of 20 -> 400 Bytes. The
	 * displacement in scatterv is multiplied by these 400 Bytes (as this
	 * is one unit of the created chunk-datatype).
	 * Because of that you can't address the start of the 2. chunk. This
	 * would starts at Byte 20 of the input_matrix.
	 */

	MPI_Type_vector(dim_m, normal_n, dim_n, MPI_UNSIGNED_CHAR, &normal_dt);
	MPI_Type_commit(&normal_dt);

	MPI_Type_vector(dim_m, remain_n, dim_n, MPI_UNSIGNED_CHAR, &remain_dt);
	MPI_Type_commit(&remain_dt);

	for (i = 0; i < topo_dims[0]; i++) {
		for (j = 0; j < (topo_dims[1] - 1); j++) {
			if(i == pdata->coords[0] && j == pdata->coords[1])
				continue;

			__send_mat_chunk(input_matrix, i, j, dim_m, normal_n,
					&normal_dt, &pdata->topo);
		}

		if(i == pdata->coords[0] && j == pdata->coords[1])
			continue;

		__send_mat_chunk(input_matrix, i, j, dim_m, normal_n,
				&remain_dt, &pdata->topo);
	}

	/* copy root-matrix */

	for(i = 0; i < dims.m; i++)
		for(j = 0; j < dims.n; j++)
			matrix_set(pdata->matrix, i, j,
					matrix_get(input_matrix, i, j));
//	print_inputmatrix(pdata->matrix);

	/*
	 * clean up
	 */
	rc = 0;

	MPI_Type_free(&remain_dt);
	MPI_Type_free(&normal_dt);
err_create_lmatrix:
	matrix_destroy(count_matrix);
err_create_count:
	matrix_destroy(disp_matrix);
err_create_disp:
	matrix_destroy(dim_matrix);
err_create_dim:
	return rc;
}

static int mpi_receive_matrix (struct processor_data *pdata)
{
	int rc;
	struct matrix_dims dims;

	MPI_Scatterv(NULL, NULL, NULL, MPI_matrix_dims,
			&dims, 1, MPI_matrix_dims, 0, pdata->topo);

	rc = matrix_create(&pdata->matrix, dims.m, dims.n,
			sizeof(unsigned char));
	if(rc)
		goto err_out;

	MPI_Recv(pdata->matrix->matrix, matrix_size(pdata->matrix),
			MPI_UNSIGNED_CHAR, 0, MPI_TAG_MAT_DIST,
			pdata->topo, MPI_STATUS_IGNORE);

//	print_inputmatrix(pdata->matrix);

	return 0;
err_out:
	return rc;
}

static int mpi_working_function(struct processor_data *pdata, int *dims)
{
	int i, k, rc, max;
	int comm_coords[COMM_DIMS], comm_rank, comm_len = 0;
	int target_dimension = (dims[0] > 1 ? 0 : 1);
	size_t comm_size = 0;

	MPI_Status status;
	MPI_Datatype border_type;

	matrix_type *communication_border, *own_border;
	list_type *communication_lists;
	struct component_list communication_list;

	rc = list_create(&communication_lists, sizeof(communication_list));
	if(rc)
		goto err_commlists_create;

	if(target_dimension == 0) { /* bottom border has to be sent */
		MPI_Type_contiguous(pdata->matrix->n, MPI_UNSIGNED,
				&border_type);

		own_border = (matrix_type *) vector_get_value(
				pdata->borders, BORDER_BOTTOM);

		rc = matrix_create(&communication_border, 1, pdata->matrix->n,
				own_border->element_size);
	} else { /* right border has to be sent */
		/*
		 * this is maybe a little strange, but becuase the border has
		 * only one dimension, there is no need for any kind of
		 * displacement
		 */
		MPI_Type_contiguous(pdata->matrix->m, MPI_UNSIGNED,
				&border_type);

		own_border = (matrix_type *) vector_get_value(
				pdata->borders, BORDER_RIGHT);

		rc = matrix_create(&communication_border, pdata->matrix->m, 1,
				own_border->element_size);
	}
	MPI_Type_commit(&border_type);

	if(rc)
		goto err_border_create;

	/*
	 * try to recive something from the left or upper node
	 */

	comm_coords[target_dimension % COMM_DIMS] =
		pdata->coords[target_dimension % COMM_DIMS] - 1;
	comm_coords[(target_dimension + 1) % COMM_DIMS] =
		pdata->coords[(target_dimension + 1) % COMM_DIMS];

	if(comm_coords[target_dimension % COMM_DIMS] >= 0) {
		/* in case we are _not_ the first node */

		/* get the corresponding rank */
		MPI_Cart_rank(pdata->topo, comm_coords, &comm_rank);

		/* get the count of component_lists to receive */
		MPI_Recv(&comm_size, 1, MPI_UNSIGNED, comm_rank,
				MPI_TAG_COMLIST_SIZE, pdata->topo, &status);

		/* receive the lists */
		for(k = 0; k < comm_size; k++) {
			/* probe the count of components in the list */
			MPI_Probe(comm_rank, MPI_TAG_COMLIST, pdata->topo,
					&status);
			MPI_Get_count(&status, MPI_component_type, &comm_len);

			/* allocate enougth memory to save to components */
			rc = vector_create(&communication_list.components,
					comm_len, sizeof(struct component));
			if(rc)
				goto err_compvector_create;

			/* get the list */
			MPI_Recv(communication_list.components->values,
					comm_len, MPI_component_type, comm_rank,
					MPI_TAG_COMLIST, pdata->topo, &status);

			/* set the count of wirtten elements in the vector */
			communication_list.components->elements = comm_len;

			/* add it to the comm_comp_list */
			rc = list_append(communication_lists,
						&communication_list);
			if(rc)
				goto err_comp_append;
		}

		/* receive the border */
		MPI_Recv(communication_border->matrix, 1, border_type,
				comm_rank, MPI_TAG_BORDER, pdata->topo,
				&status);

		fprintf(stdout, "\nlists from rank %d:\n", comm_rank);
		print_component_lists(communication_lists);

		fprintf(stdout, "\nborder from rank %d:\n", comm_rank);
		print_border(communication_border, target_dimension);
	}

	/*
	 * try to send own component_lists to the right or bottom node
	 */

	comm_coords[target_dimension % COMM_DIMS] =
		pdata->coords[target_dimension % COMM_DIMS] + 1;
	comm_coords[(target_dimension + 1) % COMM_DIMS] =
		pdata->coords[(target_dimension + 1) % COMM_DIMS];

	if(comm_coords[target_dimension % COMM_DIMS] <
			dims[target_dimension % COMM_DIMS]) {
		/* in case we are _not_ the last node */

		MPI_Cart_rank(pdata->topo, comm_coords, &comm_rank);

		/* send count of component_lists */
		MPI_Send(&pdata->component_lists->elements, 1, MPI_UNSIGNED,
				comm_rank, MPI_TAG_COMLIST_SIZE, pdata->topo);

		for(k = 0; k < pdata->component_lists->elements; k++) {
			list_get(pdata->component_lists, k,
					&communication_list);

			MPI_Send(communication_list.components->values,
					communication_list.components->elements,
					MPI_component_type, comm_rank,
					MPI_TAG_COMLIST, pdata->topo);
		}

		/* send the border */
		MPI_Send(own_border->matrix, 1, border_type, comm_rank,
				MPI_TAG_BORDER, pdata->topo);
	}

	/* cleanup */
	rc = 0;
err_compvector_create:
err_comp_append:
	matrix_destroy(communication_border);
err_border_create:
	MPI_Type_free(&border_type);

	max = communication_lists->elements;
	for (i = 0; i < max; i++) {
		list_get_head(communication_lists, &communication_list);
		list_remove_head(communication_lists);
		vector_destroy(communication_list.components);
	}
	list_destroy(communication_lists);
err_commlists_create:
	return rc;
}

