#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <assert.h>

#include "list.h"
#include "queue.h"
#include "vector.h"
#include "stack.h"
#include "matrix.h"
#include "components.h"
#include "algorithm.h"
#include "border_compare.h"

int debug_func(unsigned int id) {
	return id;
}

#define debug_if(condition) if(condition && fprintf(stderr, "ERR in %s:%d:%s(): %s\n", __FILE__, __LINE__, __func__, strerror(rc)))

static char * __print_composite_own(struct composite_own *c)
{
	static char str[256];

	sprintf(str, "(own_cid.. %d; merge_target.. %d)",
			c->own_cid, c->merge_target);

	return str;
}

static char * __print_composite_alien(struct composite_alien *c)
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

int compalien_compare_m(const void *lhv, const void *rhv, size_t size)
{
	struct composite_alien *lhc = (struct composite_alien *) lhv;
	struct composite_alien *rhc = (struct composite_alien *) rhv;

	if(lhc->merge_target < rhc->merge_target)
		return -1;
	else if(lhc->merge_target > rhc->merge_target)
		return 1;
	else
		return 0;
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

int compown_compare_m(const void *lhv, const void *rhv, size_t size)
{
	struct composite_own *lhc = (struct composite_own *) lhv;
	struct composite_own *rhc = (struct composite_own *) rhv;

	if(lhc->merge_target < rhc->merge_target)
		return -1;
	else if(lhc->merge_target > rhc->merge_target)
		return 1;
	else
		return 0;
}

static int __test_vectors(
		vector_type *merged_alien, vector_type *idx_mergedalien_m,
		vector_type *merged_own, vector_type *idx_mergedown_m)
{
	int rc;

	rc = vector_is_sorted(merged_alien);
	if(rc != 1) {
		fprintf(stderr, "__test_vectors: merged_alien is unsorted\n");
		goto err_unsorted;
	}

	rc = vector_is_sorted(idx_mergedalien_m);
	if(rc != 1) {
		fprintf(stderr, "__test_vectors: idx_mergedalien_m is unsorted\n");
		goto err_unsorted;
	}

	rc = vector_is_sorted(merged_own);
	if(rc != 1) {
		fprintf(stderr, "__test_vectors: merged_own is unsorted\n");
		goto err_unsorted;
	}

	rc = vector_is_sorted(idx_mergedown_m);
	if(rc != 1) {
		fprintf(stderr, "__test_vectors: idx_mergedown_m is unsorted\n");
		goto err_unsorted;
	}

	return 0;
err_unsorted:
	return EFAULT;
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

	rc = vector_create(&init, init_size, sizeof(struct composite_alien));
	debug_if(rc)
		goto err_idx_mergedalien_create;
	init->compare = compalien_compare_m;
	*idx_mergedalien_m = init;

	rc = vector_create(&init, init_size, sizeof(struct composite_own));
	debug_if(rc)
		goto err_merged_own_create;
	init->compare = compown_compare_o;
	*merged_own = init;

	rc = vector_create(&init, init_size, sizeof(struct composite_own));
	debug_if(rc)
		goto err_idx_mergedown_create;
	init->compare = compown_compare_m;
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
	struct composite_alien compa;

//	fprintf(stderr, "__insert_comalien: %d -> %d\n", alien_cid, merge_target);

	compa.alien_cid = alien_cid;
	compa.merge_target = merge_target;

	rc = vector_insert_sorted(merged_alien, &compa, 0);
	debug_if(rc)
		return rc;

	rc = vector_insert_sorted(idx_mergedalien_m, &compa, 1);
	debug_if(rc)
		return rc;

	return 0;
}

static int __insert_compown(vector_type *merged_own,
		vector_type *idx_mergedown_m, unsigned int own_cid,
		unsigned int merge_target)
{
	int rc;
	struct composite_own compo;

//	fprintf(stderr, "__insert_compown: %d -> %d\n", own_cid, merge_target);

	compo.own_cid = own_cid;
	compo.merge_target = merge_target;

	rc = vector_insert_sorted(merged_own, &compo, 0);
	debug_if(rc)
		return rc;

	rc = vector_insert_sorted(idx_mergedown_m, &compo, 1);
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

//	/* DEBUGGING */
//	if((rc = __test_vectors(merged_alien, idx_mergedalien_m,
//					merged_own, idx_mergedown_m)))
//		return rc;

	return 0;
err_find_component:
err_insert_composite:
	return rc;
}

static inline int __set_merged_value(vector_type *vec, struct composite *compikey,
		unsigned int merge_target)
{
	int rc;
	struct composite *compi_p;

	compi_p = bsearch_vector(vec, compikey, NULL);
	debug_if((rc = EFAULT) && !compi_p)
		return EFAULT;

	compi_p->merge_target = merge_target;

	return 0;
}

static int __update_merged_target(vector_type *vec, vector_type *idx_m,
		unsigned int old_value,	unsigned int new_value)
{
	int rc = 0, i;
	unsigned int init_pos, min_pos, max_pos, insert_pos;

	/*
	 * we don't distinguish between _alien and _own, because we only need
	 * the merge_target-field
	 */
	struct composite compikey = {0, 0}, *compi_p;

	static vector_type *buf = NULL;

	compikey.merge_target = old_value;
	compi_p = bsearch_vector(idx_m, &compikey, &init_pos);
	if(!compi_p)
		return 0;

	if(!buf) {
		/*
		 * TODO: this is more like a hack.. if you ever want a multithreaded
		 * application, CHANGE THIS!
		 */
		rc = vector_create(&buf, idx_m->elements, idx_m->element_size);
		debug_if(rc)
			goto err_buf_create;
	}

//	fprintf(stderr, "__update_merged_target: %d -> %d\n", old_value, new_value);

	/*
	 * ok, now we know there is at least one component to changed.
	 * change all component before and after this on, which have the same
	 * merge_target and save the minmal and maximal position of this group
	 */
	min_pos = max_pos = i = init_pos;
	compikey.merge_target = old_value;

	do {
		compi_p->merge_target = new_value;
		compikey.unused = compi_p->unused;
		__set_merged_value(vec, &compikey, new_value);

		min_pos = i;

		if((--i) < 0)
			break;

		compi_p = vector_get_value(idx_m, i);
		debug_if(!compi_p)
			goto err_get_composite;
	} while (compi_p->merge_target == old_value);

	i = max_pos;
	while((++i) < idx_m->elements) {
		compi_p = vector_get_value(idx_m, i);
		debug_if(!compi_p)
			goto err_get_composite;

		if(compi_p->merge_target != old_value)
			break;

		compi_p->merge_target = new_value;
		compikey.unused = compi_p->unused;
		__set_merged_value(vec, &compikey, new_value);

		max_pos = i;
	}

	/*
	 * know all components with old_value as merge_target are update,
	 * but we need to reorder the index. We have to move the whole group
	 * [min_pos, max_pos] to a sorted position.
	 */
	compikey.unused = 0;
	compikey.merge_target = 0;

	insert_pos = -1;
	/* look at the elements before the group */
	i = min_pos - 1;
	if(i >= 0) {
		compi_p = vector_get_value(idx_m, i);
		debug_if(!compi_p)
			goto err_get_composite;

		if(compi_p->merge_target > new_value) {
			/*
			 * the new position of our group is
			 * before ths current position
			 */
			compikey.merge_target = new_value;

			/* search in this speacial area */
			insert_pos = bsearch_vector_sortedpos(
					idx_m, &compikey, 0, i);
		}
	}

	i = max_pos + 1;
	if(i < idx_m->elements) {
		compi_p = vector_get_value(idx_m, i);
		debug_if(!compi_p)
			goto err_get_composite;

		if(compi_p->merge_target < new_value) {
			/*
			 * the new position of our group is
			 * before ths current position
			 */
			compikey.merge_target = new_value;

			/* search in this speacial area */
			insert_pos = bsearch_vector_sortedpos(
					idx_m, &compikey, i,
					idx_m->elements - 1);
		}
	}

	if(insert_pos != -1) {
		/*
		 * we found a new location for our group,
		 * so lets move it there
		 */
		rc = vector_massmove(idx_m, min_pos, max_pos,
				insert_pos, buf);
		debug_if(rc)
			goto err_massmove;
	}

	return 0;
err_buf_create:
err_massmove:
err_get_composite:
	return rc;
}

static int __merge_own(struct component_list *owncompl,
		struct component_list *aliencompl, unsigned int owncid,
		unsigned int aliencid, unsigned int alienmerge_cid,
		vector_type *merged_alien, vector_type *idx_mergedalien_m,
		vector_type *merged_own, vector_type *idx_mergedown_m)
{
	int rc;
	unsigned int owncomp_pos;

	struct component *own_comp, *merge_comp, compkey = {{0, 0}, 0, 0};
	struct composite_own compokey = {0, 0}, *compo_p;

	/* get our own component */
	compkey.component_id = owncid;
	own_comp = bsearch_vector(owncompl->components,	&compkey, &owncomp_pos);
	if(!own_comp) {
		/*
		 * ok, the cid found on the right side doesn't exist
		 * anymore, this happens if we already merged this component
		 * of our own with some other component of our own. Lets
		 * test this.
		 */

		compokey.own_cid = owncid;
		compo_p = bsearch_vector(merged_own, &compokey, NULL);
		debug_if((rc = EFAULT) && !compo_p)
			goto err_find_composite;

		/* these two are already merged */
		if(compo_p->merge_target == alienmerge_cid)
			return 0;

//		fprintf(stderr, "(%d, %d, %d)\n", aliencid, owncid, alienmerge_cid);

		compkey.component_id = compo_p->merge_target;
		own_comp = bsearch_vector(owncompl->components, &compkey,
				&owncomp_pos);
		debug_if((rc = EFAULT) && !own_comp)
			goto err_find_component;
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

	rc = __update_merged_target(merged_own, idx_mergedown_m,
			own_comp->component_id,	merge_comp->component_id);
	debug_if(rc)
		goto err_update_mtarget;

	rc = __update_merged_target(merged_alien, idx_mergedalien_m,
			own_comp->component_id,	merge_comp->component_id);
	debug_if(rc)
		goto err_update_mtarget;

	/*
	 * delete the merge component out of our own list,
	 * to save search time
	 */
	vector_del_value(owncompl->components, owncomp_pos);

//	/* DEBUGGING */
//	if((rc = __test_vectors(merged_alien, idx_mergedalien_m,
//					merged_own, idx_mergedown_m)))
//		return rc;

	return 0;
err_update_mtarget:
err_find_component:
err_find_composite:
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

	vector_type *merged_alien = NULL;
	vector_type *idx_mergedalien_m = NULL;

	vector_type *merged_own = NULL;
	vector_type *idx_mergedown_m = NULL;

	rc = __init_composite_vectors(matrix_size(alien_border) / 2.5,
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

//	fprintf(stdout, "\nmerged alien components:\n");
//	for(i = 0; i < merged_alien->elements; i++) {
//		compa_p = vector_get_value(merged_alien, i);
//		fprintf(stdout, "%s\n", __print_composite_alien(compa_p));
//	}
//
//	fprintf(stdout, "\nmerged own components:\n");
//	for(i = 0; i < merged_own->elements; i++) {
//		compo_p = vector_get_value(merged_own, i);
//		fprintf(stdout, "%s\n", __print_composite_own(compo_p));
//	}

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
