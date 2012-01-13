#ifndef COMPONENTS_H
#define COMPONENTS_H

#define COMM_DIMS 2

#include <stdio.h>

#include "vector.h"

struct component {
	int proc_rank;
	/* [0] = i; [1] = j; matrix-dimension-related */
	unsigned int example_coords[COMM_DIMS];
	unsigned int size;
	unsigned int component_id;
} __attribute__((__packed__));

/* packed because this shall be transmitted via mpi */

int component_create(struct component ** comp)
	__attribute__((warn_unused_result));
void component_destroy(struct component * comp);

struct component_list {
	/* containes 'struct component' */
	vector_type *components;
};

int component_list_create(struct component_list ** compl)
	__attribute__((warn_unused_result));
void component_list_destroy(struct component_list * compl);

/**
 * find_components() -	searches a matrix of black and white fields for black
 *			components (sets of connected black fields)
 * @mat:	the target-matrix
 * @comp_list:	a _un_initialized component list
 * @borders:	optional vector (_un_initialized)
 *
 * Returns %0 in success, any other value indicates a value
 * (see errno.h macros).
 *
 * On success, @comp_list will contain a list of the found components
 * (see struct component_list).
 * If @borders is not %NULL, it will contain a vector that contains all 4
 * borders of @mat, with each field containing the number of the found
 * component or %0 if no component was found for this field. The number
 * addresses the index in @comp_list.
 *
 * @comp_list and @borders need to be destroyed if not needed anymore.
 **/
int find_components(matrix_type *mat, struct component_list **comp_list,
		vector_type **borders)
	__attribute__((warn_unused_result));

/* on return the returned structs are as it follows:
 *
 * mat -> contains the input-matrix
 *
 * (*comp_list), is a (struct component_list *) which contains a vector_type
 *	-> components, which contains all found components, ordered by their
 *			component_id. The elements of components are of the
 *			type (struct component)
 *
 * usage example:
 *	int i;
 *	struct component *comp_p;
 *
 *	fprintf(stdout, "components:\n");
 *	for(i = 0; i < comp_list->components->elements; i++) {
 *		comp_p = (struct component *) vector_get_value(
 *				components->components, i);
 *		fprintf(stderr, "%u\n", comp_p->size);
 *	}
 *
 * (*borders), is a vector_type, which contains all 4 borders of the matrix
 *	-> each elements is of the type matrix_type, and is a matrix with the
 *		dimension of the corresponding matrix-border
 *	-> each field in one border contains the component_id of the found
 *		component, and 0 if no comp. was found. The fields are of the
 *		type (unsigned int)
 *	-> the borders in the vector are indexed by the enum-values of
 *		(enum border_nr)
 *
 * usage example:
 *	int k, i, j;
 *	matrix_type *border;
 *
 *	for(k = BORDER_MIN; k < BORDER_MAX; k++) {
 *		border = vector_get_value(borders, k);
 *		if(!border) // strange
 *			continue;
 *
 *		fprintf(stdout, "%s: \n", strborder(k));
 *
 *		switch(k) {
 *		case BORDER_TOP:
 *		case BORDER_BOTTOM:
 *			for(j = 0, i = 0; j < border->n; j++) {
 *				cid = (unsigned int *) matrix_get(border, i, j);
 *				if(!cid)
 *					fprintf(stdout, "  , ");
 *				else
 *					fprintf(stdout, "%2d, ", *cid);
 *			}
 *			break;
 *		case BORDER_LEFT:
 *		case BORDER_RIGHT:
 *			for(j = 0, i = 0; i < border->m; i++) {
 *				cid = (unsigned int *) matrix_get(border, i, j);
 *				if(!cid)
 *					fprintf(stdout, "  , ");
 *				else
 *					fprintf(stdout, "%2d, ", *cid);
 *			}
 *			break;
 *		}
 *	}
 *
 * (*borders) and (*comp_list) are unchanged if find_components returns
 * something != 0
 *
 * both should be freed with either borders_destroy or component_list_destroy
 */

enum border_nr {
	BORDER_MIN	= 0,
	BORDER_LEFT	= 0,
	BORDER_TOP	= 1,
	BORDER_RIGHT	= 2,
	BORDER_BOTTOM	= 3,
	BORDER_MAX
};

char * strborder(enum border_nr nr);

/* should be used to destroy the borders-variable of find_components */
void borders_destroy(vector_type *borders);

#endif // COMPONENTS_H
