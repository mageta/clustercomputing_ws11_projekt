#ifndef BORDER_COMPARE_H
#define BORDER_COMPARE_H

#include <stdio.h>

#include "vector.h"
#include "matrix.h"
#include "components.h"

/* composites of two components */
struct composite {
	unsigned int unused;
	unsigned int merge_target;	/* N */
};

struct composite_own {
	unsigned int own_cid;		/* 1 */
	unsigned int merge_target;	/* N */
};

struct composite_alien {
	unsigned int alien_cid;		/* 1 */
	unsigned int merge_target;	/* N */
};

/*
 * compare-functions to sort the component-vectors,
 * so we can search in logarithmic-time-complexity in these vectors
 */

/* search composite_alien after their alien-cid */
int compalien_compare_a(const void *lhv, const void *rhv, size_t size);
/* search composite_alien after their merge-target */
int compalien_compare_m(const void *lhv, const void *rhv, size_t size);

/* search composite_own after their own-cid */
int compown_compare_o(const void *lhv, const void *rhv, size_t size);
/* search composite_own after their merge-target */
int compown_compare_m(const void *lhv, const void *rhv, size_t size);

/**
 * find_common_components() - tries to merge all components from
 * 	@communication_compl into the own component-list @own_compl
 * @own_compl: the list of the own components
 * @communication_compl: the list of the received/alien components
 * @compare_border: the border of our own that should be used to be
 *	compared with @alien_border
 * @send_border: the border of our own that later shall be sent to the next
 *	processor
 * @alien_border: the received/alien border that should be compared with
 *	@compare_border
 *
 * The algorithm compares @compare_border and @alien_border and searches for
 * components which are connected. If it finds such components, it merges them.
 * All other components (alien-components which are not connected) are also
 * added to @own_compl.
 *
 * On success @own_compl shall only contain components which either are
 * merged or singular in @own_compl/@communication_compl. @send_border will
 * be updated in case that two components of @own_compl are merged.
 * @communication_compl is changed during this process.
 *
 * Returns %0 in success, any other value indicates a value
 * (see errno.h macros).
 **/
int find_common_components(struct component_list *own_compl,
		struct component_list *communication_compl,
		matrix_type *compare_border, matrix_type *send_border,
		matrix_type *alien_border);

#endif // BORDER_COMPARE_H
