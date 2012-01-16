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
#include "algorithm.h"

#define MPI_TAG_BASE		0
#define MPI_TAG_COMLIST_SIZE	MPI_TAG_BASE + 1
#define MPI_TAG_COMLIST		MPI_TAG_COMLIST_SIZE + 1
#define MPI_TAG_MAT_DIST	MPI_TAG_COMLIST + 1
#define MPI_TAG_BORDER		MPI_TAG_MAT_DIST + 1
#define MPI_TAG_MAT_DIMS	MPI_TAG_BORDER + 1

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
//	list_type *component_lists;
	struct component_list *comp_list;
	vector_type *borders;
	matrix_type *matrix;
};

struct matrix_dims {
	unsigned int m, n;
} __attribute__((__packed__));

static int		rid = 0;
static MPI_Request	*requests;
static MPI_Status	*status;

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
static void	print_component_list		(struct component_list *list);
static void	print_inputmatrix		(matrix_type *matrix);
static void	print_border			(matrix_type *border,
						 int dimension);
static int	find_common_components		(struct processor_data *pdata,
						 struct component_list
							*communication_list,
						 matrix_type *own_border,
						 matrix_type
							*communication_border);

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

	matrix_type *input_matrix;

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

//	rc = list_create(&pdata.component_lists,
//			sizeof(struct component_list));
//	if(rc) goto err_output;

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
			goto err_fileread;
		}
		rc = mpi_distribute_matrix(&pdata, dims, input_matrix);
	} else {
		rc = mpi_receive_matrix(&pdata);
	}

	if(rc)
		goto err_matrix_distrib;

	/*
	 * the matrix-chunk is now available in pdata.matrix
	 * lets find the local components
	 */

	rc = find_components(pdata.matrix, &pdata.comp_list, &pdata.borders);
	if(rc)
		goto err_find_comp;

	print_component_list(pdata.comp_list);

	/* call main working function */

	rc = mpi_working_function(&pdata, dims);
	if(rc)
		goto err_free_list;

	/*
	 * clean up
	 */
	rc = 0;
err_free_list:
	component_list_destroy(pdata.comp_list);
//	borders_destroy(pdata.borders);
err_find_comp:
	matrix_destroy(pdata.matrix);
err_matrix_distrib:
	MPI_Comm_free(&pdata.topo);
err_fileread:
	MPI_Type_free(&MPI_matrix_dims);
	MPI_Type_free(&MPI_component_type);

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

static void print_component_list(struct component_list *list)
{
	int i;
	struct component *comp;

	fprintf(stdout, "clist:\n");

	for(i = 0; i < list->components->elements; i++) {
		comp = (struct component *) vector_get_value(
					list->components, i);

		fprintf(stdout, "comp %d, size %d, (%d, %d)\n",
				comp->component_id, comp->size,
				comp->example_coords[0],
				comp->example_coords[1]);
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
			fprintf(stdout, (val? "%hhd, " : "%hhd, "), val);
		}
		val = *((unsigned char *) matrix_get(matrix, i, j));
		fprintf(stdout, (val? "%hhd\n" : "%hhd\n"), val);
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

static int mpi_distribute_matrix (struct processor_data *pdata,
		int *topo_dims, matrix_type *input_matrix)
{
	int rc, i, j, disp;
	unsigned int dim_m, dim_n;
	unsigned int normal_n, remain_n;
	double n_per_proc;

	int comm_coords[COMM_DIMS];
	int comm_rank;
	void * comm_mat_begin;

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

	requests = calloc(pdata->topo_size - 1, sizeof(*requests));
	status = calloc(pdata->topo_size - 1, sizeof(*status));

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
		for (j = 0; j < topo_dims[1]; j++) {
			if(i == pdata->coords[0] && j == pdata->coords[1]) {
				continue;
			}

			comm_coords[0] = i;
			comm_coords[1] = j;
			MPI_Cart_rank(pdata->topo, comm_coords, &comm_rank);

			comm_mat_begin = matrix_get(input_matrix, i * dim_m,
					j * normal_n);

			MPI_Isend(comm_mat_begin, 1, (j == (topo_dims[1] - 1) ?
					remain_dt : normal_dt), comm_rank,
					MPI_TAG_MAT_DIST, pdata->topo,
					&requests[rid++]);
		}
	}

	/* create the space for the local matrix */
	rc = matrix_create(&pdata->matrix, dims.m, dims.n,
			sizeof(unsigned char));
	if(rc)
		goto err_create_lmatrix;

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
	int rc = 0;
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
	int rc;
	int comm_coords[COMM_DIMS], comm_rank, comm_len = 0;
	int target_dimension = (dims[0] > 1 ? 0 : 1);

	MPI_Status status;
	MPI_Datatype border_type;

	matrix_type *communication_border, *own_border, *compare_border;
	struct component_list communication_list = { .components = NULL };

	if(target_dimension == 0) { /* bottom border has to be sent */
		MPI_Type_contiguous(pdata->matrix->n, MPI_UNSIGNED,
				&border_type);

		own_border = (matrix_type *) vector_get_value(
				pdata->borders, BORDER_BOTTOM);
		compare_border = (matrix_type *) vector_get_value(
				pdata->borders, BORDER_TOP);

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
		compare_border = (matrix_type *) vector_get_value(
				pdata->borders, BORDER_LEFT);

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

		/* probe the count of components */
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

		/* receive the border */
		MPI_Recv(communication_border->matrix, 1, border_type,
				comm_rank, MPI_TAG_BORDER, pdata->topo,
				&status);

		fprintf(stdout, "\nlists from rank %d:\n", comm_rank);
		print_component_list(&communication_list);

		fprintf(stdout, "\nborder from rank %d:\n", comm_rank);
		print_border(communication_border, target_dimension);

		rc = find_common_components(pdata, &communication_list,
				compare_border, communication_border);
		if(rc)
			goto err_find_comcomp;
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

		/* send the own component-list */
		MPI_Send(pdata->comp_list->components->values,
				pdata->comp_list->components->elements,
				MPI_component_type, comm_rank,
				MPI_TAG_COMLIST, pdata->topo);

		/* send the border */
		MPI_Send(own_border->matrix, 1, border_type, comm_rank,
				MPI_TAG_BORDER, pdata->topo);
	}

	/* cleanup */
	rc = 0;
err_find_comcomp:
err_compvector_create:
	matrix_destroy(communication_border);
err_border_create:
	MPI_Type_free(&border_type);
	vector_destroy(communication_list.components);
	return rc;
}

static int component_compare(const void *lhv, const void *rhv, size_t size)
{
	struct component *lhc = (struct component *) lhv;
	struct component *rhc = (struct component *) rhv;

	if(lhc->component_id < rhc->component_id)
		return -1;
	else if(lhc->component_id > rhc->component_id)
		return 1;
	else
		return 0;
}

struct component_composite {
	unsigned int alien_cid,
		     own_cid,
		     merge_target;
};

static int compcomposite_compare(const void *lhv, const void *rhv, size_t size)
{
	struct component_composite *lhc = (struct component_composite *) lhv;
	struct component_composite *rhc = (struct component_composite *) rhv;

	if(lhc->alien_cid < rhc->alien_cid)
		return -1;
	else if(lhc->alien_cid > rhc->alien_cid)
		return 1;
	else {
		if(rhc->own_cid == 0)
			return 0;

		if(lhc->own_cid < rhc->own_cid)
			return -1;
		else if(lhc->own_cid > rhc->own_cid)
			return 1;
		else {
			if(rhc->merge_target == 0)
				return 0;

			if(lhc->merge_target < rhc->merge_target)
				return -1;
			else if(lhc->merge_target > rhc->merge_target)
				return 1;
			else
				return 0;
		}
	}
}

static int find_common_components(struct processor_data *pdata,
		struct component_list *communication_list,
		matrix_type *own_border,
		matrix_type *communication_border)
{
	int rc, i, j, k;
	unsigned int pos;
	unsigned int rcid, lcid;

	struct component *alien_comp, *own_comp, *merge_comp, compkey;
	struct component_composite composite, compikey, *compi_p;
	vector_type *composites;

	rc = vector_create(&composites, (matrix_size(own_border) / 3) + 1,
			sizeof(struct component_composite));
	if(rc)
		goto err_composites_create;

	memset(&compkey, 0, sizeof(compkey));
	memset(&compikey, 0, sizeof(compikey));

	composites->compare = compcomposite_compare;

	pdata->comp_list->components->compare = component_compare;
	communication_list->components->compare = component_compare;

	for(i = 0, j = 0; i < communication_border->m; i++) {
		lcid = *((unsigned int *) matrix_get(communication_border,
					i, j));

		if(!lcid)
			continue;

		/* get the component with this id */
		compkey.component_id = lcid;
		alien_comp = bsearch_vector(communication_list->components,
				&compkey, NULL);
		if(!alien_comp) {
			rc = EFAULT;
			goto err_find_component;
		}

		/* look at the neighbour-field in our own border */
		for (k = -1; k < 2; k++) {
			if(!matrix_index_valid(own_border, i + k, j))
				continue;

			rcid = *((unsigned int *) matrix_get(own_border,
						i + k, j));

			if(!rcid)
				continue;

			/* get the component with this id */
			compkey.component_id = rcid;
			own_comp = bsearch_vector(
					pdata->comp_list->components,
					&compkey, NULL);
			if(!alien_comp) {
				rc = EFAULT;
				goto err_find_component;
			}

			/*
			 * test if we already merged this component with
			 * something
			 */
			compikey.alien_cid = lcid;

			compi_p = bsearch_vector(composites, &compikey, &pos);
			if(!compi_p) {
				/* no, it was never merged; do it now */

//				own_comp->size += alien_comp->size;

				/* save this composite */
				composite.alien_cid = lcid;
				composite.own_cid = rcid;
				composite.merge_target = rcid;

				rc = vector_insert_sorted(composites,
						&composite, 0);
				if(rc)
					goto err_insert_composite;

				continue;
			}

			/* yes we merged this component already */

			if(compi_p->merge_target == rcid) {
				/* we merge these two components already */
				continue;
			}

			/*
			 * the alien component connects two spearate
			 * component of our own
			 *
			 * we have to merge two of our own components
			 */

			compkey.component_id = compi_p->merge_target;
			merge_comp = bsearch_vector(
					pdata->comp_list->components,
					&compkey, NULL);
			if(!alien_comp) {
				rc = EFAULT;
				goto err_find_component;
			}

			/* save this composite */
			composite.alien_cid = lcid;
			composite.own_cid = rcid;
			composite.merge_target = compi_p->merge_target;

			rc = vector_insert_sorted(composites,
					&composite, 0);
			if(rc)
				goto err_insert_composite;
		}
	}

	fprintf(stdout, "\nmerged components:\n");
	for(i = 0; i < composites->elements; i++) {
		compi_p = vector_get_value(composites, i);

		fprintf(stdout, "(alien_cid: %d, own_cid: %d, merged_cid:"
				" %d)\n",
				compi_p->alien_cid, compi_p->own_cid,
				compi_p->merge_target);
	}

	return 0;
err_find_component:
err_insert_composite:
	vector_destroy(composites);
err_composites_create:
	return rc;
}
