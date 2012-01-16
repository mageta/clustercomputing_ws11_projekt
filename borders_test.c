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

int main(int argc, char **argv)
{
	/*
	 * MPI-Variablen:
	 * p	- Anzahl Prozesse
	 */
	int rc = 0;

	struct processor_data pdata;

	matrix_type *input_matrix;

	srand(time(NULL));

	if(read_input_file(&input_matrix, argv[1])) {
		fprintf(stderr, "\n%s\n", usage(argc, argv));
		goto err_output;
	}

	rc = find_components(input_matrix, &pdata.comp_list, &pdata.borders);
	if(rc)
		goto err_free_list;

	rc = 0;
err_free_list:
	borders_destroy(pdata.borders);
	component_list_destroy(pdata.comp_list);
	matrix_destroy(input_matrix);
err_output:
	return rc;
}
