#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>

#include "list.h"
#include "queue.h"
#include "vector.h"
#include "stack.h"
#include "matrix.h"
#include "components.h"
#include "algorithm.h"
#include "border_compare.h"

#define debug_if(condition) if(condition && fprintf(stderr, "ERR in %s:%d:%s(): %s\n", __FILE__, __LINE__, __func__, strerror(rc)))

static char * __printf_composite_own(struct composite_own *c)
{
	static char str[256];

	sprintf(str, "(own_cid.. %d; merge_target.. %d)",
			c->own_cid, c->merge_target);

	return str;
}

static char * __printf_composite_alien(struct composite_alien *c)
{
	static char str[256];

	sprintf(str, "(alien_cid.. %d; merge_target.. %d)",
			c->alien_cid, c->merge_target);

	return str;
}

int compalien_compare_a(const void *lhv, const void *rhv, size_t size)
{
	struct composite_alien *lhc = (struct composite_alien *) lhv;
	struct composite_alien *rhc = (struct composite_alien *) rhv;

	if(lhc->alien_cid < rhc->alien_cid)
		return -1;
	else if(lhc->alien_cid > rhc->alien_cid)
		return 1;
	else
		return 0;
}

int compalien_p_compare_m(const void *lhv, const void *rhv, size_t size)
{
	struct composite_alien *lhc = *((struct composite_alien **) lhv);
	struct composite_alien *rhc = *((struct composite_alien **) rhv);

	if(lhc->merge_target < rhc->merge_target)
		return -1;
	else if(lhc->merge_target > rhc->merge_target)
		return 1;
	else {
		if(rhc->alien_cid == 0)
			return 0;

		if(lhc->alien_cid < rhc->alien_cid)
			return -1;
		else if(lhc->alien_cid > rhc->alien_cid)
			return 1;
		else
			return 0;
	}
}

int compown_compare_o(const void *lhv, const void *rhv, size_t size)
{
	struct composite_own *lhc = (struct composite_own *) lhv;
	struct composite_own *rhc = (struct composite_own *) rhv;

	if(lhc->own_cid < rhc->own_cid)
		return -1;
	else if(lhc->own_cid > rhc->own_cid)
		return 1;
	else
		return 0;
}

int compown_p_compare_m(const void *lhv, const void *rhv, size_t size)
{
	struct composite_own *lhc = *((struct composite_own **) lhv);
	struct composite_own *rhc = *((struct composite_own **) rhv);

	if(lhc->merge_target < rhc->merge_target)
		return -1;
	else if(lhc->merge_target > rhc->merge_target)
		return 1;
	else {
		if(rhc->own_cid == 0)
			return 0;

		if(lhc->own_cid < rhc->own_cid)
			return -1;
		else if(lhc->own_cid > rhc->own_cid)
			return 1;
		else
			return 0;
	}
}

static int __init_composite_vectors(size_t init_size,
		vector_type **merged_alien, vector_type **idx_mergedalien_m,
		vector_type **merged_own, vector_type **idx_mergedown_m)
{
	int rc;
	vector_type *init;

	/* allocation of the memory/search-indexes */
	rc = vector_create(&init, init_size, sizeof(struct composite_alien));
	debug_if(rc)
		goto err_merged_alien_create;
	init->compare = compalien_compare_a;
	*merged_alien = init;

	rc = vector_create(&init, init_size, sizeof(struct composite_alien *));
	debug_if(rc)
		goto err_idx_mergedalien_create;
	init->compare = compalien_p_compare_m;
	*idx_mergedalien_m = init;

	rc = vector_create(&init, init_size, sizeof(struct composite_own *));
	debug_if(rc)
		goto err_merged_own_create;
	init->compare = compown_compare_o;
	*merged_own = init;

	rc = vector_create(&init, init_size, sizeof(struct composite_own *));
	debug_if(rc)
		goto err_idx_mergedown_create;
	init->compare = compown_p_compare_m;
	*idx_mergedown_m = init;

	return 0;
err_idx_mergedown_create:
	vector_destroy(*merged_own);
err_merged_own_create:
	vector_destroy(*idx_mergedalien_m);
err_idx_mergedalien_create:
	vector_destroy(*merged_alien);
err_merged_alien_create:
	return rc;
}

static void __destroy_composite_vectors(
		vector_type *merged_alien, vector_type *idx_mergedalien_m,
		vector_type *merged_own, vector_type *idx_mergedown_m)
{
	vector_destroy(merged_alien);
	vector_destroy(idx_mergedalien_m);
	vector_destroy(merged_own);
	vector_destroy(idx_mergedown_m);
}

static int __insert_compalien(vector_type *merged_alien,
		vector_type *idx_mergedalien_m, unsigned int alien_cid,
		unsigned int merge_target)
{
	int rc;
	unsigned int pos;
	struct composite_alien compa, *compa_p;

	compa.alien_cid = alien_cid;
	compa.merge_target = merge_target;

	rc = vector_insert_sorted_pos(merged_alien, &compa, 0, &pos);
	debug_if(rc)
		return rc;

	compa_p = vector_get_value(merged_alien, pos);
	rc = vector_insert_sorted(idx_mergedalien_m, &compa_p, 0);
	debug_if(rc)
		return rc;

	return 0;
}

static int __insert_compown(vector_type *merged_own,
		vector_type *idx_mergedown_m, unsigned int own_cid,
		unsigned int merge_target)
{
	int rc;
	unsigned int pos;
	struct composite_own compo, *compo_p;

	compo.own_cid = own_cid;
	compo.merge_target = merge_target;

	rc = vector_insert_sorted_pos(merged_own, &compo, 0, &pos);
	debug_if(rc)
		return rc;

	compo_p = vector_get_value(merged_own, pos);
	rc = vector_insert_sorted(idx_mergedown_m, &compo_p, 0);
	debug_if(rc)
		return rc;

	return 0;
}

static int __merge_alien(struct component_list *owncompl,
		struct component_list *aliencompl,
		unsigned int owncid, unsigned int aliencid,
		vector_type *merged_alien, vector_type *idx_mergedalien_m,
		vector_type *merged_own, vector_type *idx_mergedown_m)
{
	int rc;
	unsigned int pos;

	struct composite_own compokey = {0, 0}, *compo_p;
	struct component *alien_comp, *own_comp, compkey = {{0, 0}, 0, 0};

	/*
	 * get the alien component (this is only possible if the component was
	 * never merged, as we remove it after the merge [to save search-time])
	 */
	compkey.component_id = aliencid;
	alien_comp = bsearch_vector(aliencompl->components, &compkey, &pos);
	debug_if((rc = EFAULT) && !alien_comp)
		goto err_find_component;

	/*
	 * now we have to find our own component, but it is possible that we
	 * merge the component with this cid already with an other of our own
	 * (the case, if a alien-component connects two of our own). So we
	 * have to serach the search-index merged_own for this cid.
	 */

	compokey.own_cid = owncid;
	compo_p = bsearch_vector(merged_own, &compokey, NULL);
	if(compo_p) {
		/* yes it was merged with an other of our own */
		compkey.component_id = compo_p->merge_target;
	} else {
		/* no, it is still available */
		compkey.component_id = owncid;
	}

	/* get the found component */
	own_comp = bsearch_vector(owncompl->components, &compkey, NULL);
	debug_if((rc = EFAULT) && !own_comp)
		goto err_find_component;

	/* accumulate the sizes */
	own_comp->size += alien_comp->size;

	/* save this composite-relation */
	rc = __insert_compalien(merged_alien, idx_mergedalien_m,
			alien_comp->component_id, own_comp->component_id);
	debug_if(rc)
		goto err_insert_composite;

	/*
	 * delete this alien component from communication_list, we don't need
	 * it anymore.
	 *
	 * if an other border-field with its id comes up, compi_p won't be NULL
	 */
	vector_del_value(aliencompl->components, pos);

	return 0;
err_find_component:
err_insert_composite:
	return rc;
}

static int __merge_own(struct component_list *owncompl,
		struct component_list *aliencompl, unsigned int owncid,
		unsigned int aliencid, unsigned int alienmerge_cid,
		vector_type *merged_alien, vector_type *idx_mergedalien_m,
		vector_type *merged_own, vector_type *idx_mergedown_m)
{
	int rc;
	unsigned int pos, owncomp_pos;

	struct composite_alien compakey = {0, 0}, *compa_p;
	struct composite_own compokey = {0, 0}, *compo_p;
	struct component *own_comp, *merge_comp, compkey = {{0, 0}, 0, 0};

	/* get our own component */
	compkey.component_id = owncid;
	own_comp = bsearch_vector(owncompl->components,	&compkey, &owncomp_pos);
	if(!own_comp) {
		/*
		 * this can happen, if we already merged this
		 * component with an other of our own, so it is
		 * ok.
		 *
		 * We could also search in the componet-
		 * composite-vector for the component, but this
		 * way it saves us time
		 */
		return 0;
	}

	/* TODO: search and merge merged components */

	/* get the other of our own components, the one to which we add */
	compkey.component_id = alienmerge_cid;
	merge_comp = bsearch_vector(owncompl->components, &compkey, NULL);
	debug_if((rc = EFAULT) && !merge_comp)
		goto err_find_component;

	/* accumulate the two */
	merge_comp->size += own_comp->size;

	/* save this composite */
	rc = __insert_compown(merged_own, idx_mergedown_m,
			own_comp->component_id, merge_comp->component_id);
	debug_if(rc)
		goto err_insert_composite;

	/*
	 * now the nastiest part of this algorithm. It is possible, that we
	 * just merged a component into a other, that was itself part of a
	 * merge. All components which were merged into 'merge_comp' have to
	 * be updated regarding thei merge_target
	 */

	/* TODO */

	/*
	 * delete the merge component out of our own list,
	 * to save search time
	 */
	vector_del_value(owncompl->components, owncomp_pos);

	return 0;
err_find_component:
err_insert_composite:
	return rc;
}

int find_common_components(struct component_list *own_compl,
		struct component_list *communication_compl,
		matrix_type *compare_border, matrix_type *send_border,
		matrix_type *alien_border)
{
	int rc, i, k, maxid;
	unsigned int pos;
	unsigned int rcid, lcid;

	struct composite_alien compakey = {0, 0}, *compa_p;
	struct composite_own compokey = {0, 0}, *compo_p;
	struct component *alien_comp, *own_comp;

	vector_type *merged_alien;
	vector_type *idx_mergedalien_m;

	vector_type *merged_own;
	vector_type *idx_mergedown_m;

	rc = __init_composite_vectors(matrix_size(alien_border) / 3,
			&merged_alien, &idx_mergedalien_m,
			&merged_own, &idx_mergedown_m);
	debug_if(rc)
		goto err_init_composite_vectors;

	/*
	 * look at each field of the border that we received from our
	 * predecessor
	 */
	for(i = 0; i < matrix_size(alien_border); i++) {
		lcid = *((unsigned int *) matrix_get_linear(alien_border, i));

		/* if the field has a valid component-id */
		if(!lcid)
			continue;

		/*
		 * look at the corresponding neighbour-fields in our
		 * own border
		 */
		for (k = -1; k < 2; k++) {
			if(((i + k) < 0) ||
				((i + k) >= matrix_size(compare_border)))
				continue;

			rcid = *((unsigned int *) matrix_get_linear(
						compare_border, i + k));

			if(!rcid)
				continue;

			/*
			 * test if we already merged this component with
			 * something (to do so, search the components-vector)
			 */
			compakey.alien_cid = lcid;
			compa_p = bsearch_vector(merged_alien, &compakey, &pos);

			/* compi_p == NULL -> nothing found */
			if(!compa_p) {
				rc = __merge_alien(own_compl,
						communication_compl,
						rcid, lcid, merged_alien,
						idx_mergedalien_m, merged_own,
						idx_mergedown_m);
				debug_if(rc)
					goto err_merge_alien;
				continue;
			}

			/*
			 * yes, we merged this component already with
			 * one of out own
			 *
			 * check if the previous merge-target is the same
			 * as the current
			 */
			if(compa_p->merge_target == rcid) {
				/* we merge these two components already */
				continue;
			}

			/*
			 * the alien component connects two separate
			 * component of our own
			 *
			 * we have to merge two of our own components,
			 * we also have to update to other borders later (this
			 * component_id will be invalid afterward)
			 */

			rc = __merge_own(own_compl, communication_compl,
					rcid, lcid, compa_p->merge_target,
					merged_alien, idx_mergedalien_m,
					merged_own, idx_mergedown_m);
			debug_if(rc)
				goto err_merge_own;
		}
	}

	/*
	 * ok, now communication_list only contains components, which are not
	 * connected to any of our own. We just append these (for this, we need
	 * to change their id to a unique one in our own list).
	 */

	/* get the id of the last component in our own *sorted* list */
	own_comp = vector_get_value(own_compl->components,
			own_compl->components->elements - 1);
	maxid = own_comp->component_id;

	k = communication_compl->components->elements;
	for(i = 0; i < k; i++) {
		alien_comp = vector_get_value(communication_compl->components,
				i);
		alien_comp->component_id = ++maxid;

		/*
		 * because this id will be bigger than any of our own, we can
		 * just append it
		 */
		rc = vector_add_value(own_compl->components, alien_comp);
		debug_if(rc)
			goto err_insert_component;
	}

	/*
	 * now the only thing left, is to update the other borders, because it
	 * is possible, that we merged/deleted one of our own components and if
	 * this component is addressed by the other send_border, we have to
	 * change the id.
	 */
	for(i = 0; i < matrix_size(send_border); i++) {
		rcid = *((unsigned int *) matrix_get_linear(send_border, i));

		if(!rcid)
			continue;

		compokey.own_cid = rcid;
		compo_p = bsearch_vector(merged_own, &compokey, &pos);
		if(!compo_p) {
			/* this componend was not merged, so it still exists */
			continue;
		}

		/*
		 * this component was merged, so we need to change the border-
		 * field into the id of the merge_target
		 */
		matrix_set_linear(send_border, i, &compo_p->merge_target);
	}

	fprintf(stdout, "\nmerged alien components:\n");
	for(i = 0; i < merged_alien->elements; i++) {
		compa_p = vector_get_value(merged_alien, i);
		fprintf(stdout, "%s\n", __printf_composite_alien(compa_p));
	}

	fprintf(stdout, "\nmerged own components:\n");
	for(i = 0; i < merged_own->elements; i++) {
		compo_p = vector_get_value(merged_own, i);
		fprintf(stdout, "%s\n", __printf_composite_own(compo_p));
	}

	/* clean-up */
	rc = 0;
err_insert_component:
err_merge_own:
err_merge_alien:
	__destroy_composite_vectors(merged_alien, idx_mergedalien_m,
			merged_own, idx_mergedown_m);
err_init_composite_vectors:
	return rc;
}
