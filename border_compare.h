#ifndef BORDER_COMPARE_H
#define BORDER_COMPARE_H

#include <stdio.h>

#include "vector.h"
#include "matrix.h"
#include "components.h"

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

int compalien_compare_a(const void *lhv, const void *rhv, size_t size);
int compalien_p_compare_m(const void *lhv, const void *rhv, size_t size);

int compown_compare_o(const void *lhv, const void *rhv, size_t size);
int compown_p_compare_m(const void *lhv, const void *rhv, size_t size);

int find_common_components(struct component_list *own_compl,
		struct component_list *communication_compl,
		matrix_type *compare_border, matrix_type *send_border,
		matrix_type *alien_border);

#endif // BORDER_COMPARE_H
