#include "list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

struct list_element {
	struct list *list;
	struct list_element *next, *prev;

	void * value;
};

static struct list_element * list_element_create(size_t size)
{
	struct list_element * le;

	le = (struct list_element *) malloc(sizeof(*le));
	if(!le)
		return NULL;
	memset(le, 0, sizeof(*le));

	le->value = malloc(size);
	if(!le->value)
		goto err_out;
	memset(le->value, 0, size);

	return le;
err_out:
	free(le);
	return NULL;
}

static void list_element_destroy(struct list_element * le)
{
	free(le->value);
	free(le);
}

int list_create(list_type ** l, size_t element_size)
{
	list_type * list;

	if(!l || !element_size)
		return EINVAL;

	list = (list_type *) malloc(sizeof(*list));
	if(!list)
		return ENOMEM;
	memset(list, 0, sizeof(*list));

	list->head = NULL;
	list->tail = NULL;
	list->elements = 0;
	list->element_size = element_size;
	list->compare = memcmp;

	*l = list;

	return 0;
}

void list_destroy(list_type * list)
{
	struct list_element *le, *lep;

	if(!list)
		return;

	le = list->tail;

	while(le) {
		lep = le->prev;
		list_element_destroy(le);
		le = lep;
	}

	free(list);
}

int list_append(list_type * list, void * value)
{
	struct list_element * le;

	if(!list || !value)
		return EINVAL;

	le = list_element_create(list->element_size);
	if(!le)
		return ENOMEM;

	le->list = list;
	le->next = NULL;
	le->prev = list->tail;

	if(!le->prev) {
		list->head = le;
		list->tail = le;
	} else {
		le->prev->next = le;
		list->tail = le;
	}

	memcpy(le->value, value, list->element_size);
	list->elements++;

	return 0;
}

int list_prepend(list_type * list, void * value)
{
	struct list_element * le;

	if(!list || !value)
		return EINVAL;

	le = list_element_create(list->element_size);
	if(!le)
		return ENOMEM;

	le->list = list;
	le->next = list->head;
	le->prev = NULL;

	if(!le->next) {
		list->head = le;
		list->tail = le;
	} else {
		le->next->prev = le;
		list->head = le;
	}

	memcpy(le->value, value, list->element_size);
	list->elements++;

	return 0;
}

int list_insert_at(list_type * list, unsigned int pos, void * value)
{
	struct list_element * le;
	struct list_element * local_node;
	unsigned int i;

	if(!list || !value)
		return EINVAL;

	if(pos > list->elements)
		return EINVAL;

	if(pos == 0) /* before the first */
		return list_prepend(list, value);
	else if(pos == list->elements) /* after the last */
		return list_append(list, value);

	/* in between */
	le = list_element_create(list->element_size);
	if(!le)
		return ENOMEM;

	le->list = list;
	le->next = NULL;
	le->prev = NULL;

	local_node = list->head;

	for(i = 0; (i < pos) && local_node; i++) {
		local_node = local_node->next;
	}

	if(!local_node) /* it seems like someone manipulated the list */
		return EFAULT;

	le->next = local_node;
	le->prev = local_node->prev;

	local_node->prev = le;
	le->prev->next = le;

	memcpy(le->value, value, list->element_size);
	list->elements++;

	return 0;
}

int list_insert_sorted(list_type * list, void * value)
{
	int pos, comp;
	void * current;
	unsigned int min, max, mid;

	if(!list || !value || !list->compare)
		return EINVAL;

	if(list->elements == 0)
		return list_append(list, value);

	min = 0;
	max = list->elements - 1;

	do {
		mid = (min + max) / 2;
		/* TODO: this is very inefficient */
		current = (char *) list_element(list, mid);

		comp = list->compare(value, current, list->element_size);
		if(comp == 0)
			break;
		else if(comp < 0) {
			if((int) (mid - 1) < 0)
				break;
			max = mid - 1;
		}
		else
			min = mid + 1;
	} while (min <= max);

	if(comp > 0)
		pos = mid + 1;
	else
		pos = mid;

	return list_insert_at(list, pos, value);
}

void * list_head(list_type *list)
{
	if(!list || !list->head)
		return NULL;

	return list->head->value;
}

void * list_tail(list_type *list)
{
	if(!list || !list->tail)
		return NULL;

	return list->tail->value;
}

void * list_element(list_type *list, unsigned int pos)
{
	int i;
	struct list_element *local_node;

	if(!list)
		return NULL;

	if(pos == 0) /* the first */
		return list_head(list);
	else if((pos + 1) == list->elements) /* the last */
		return list_tail(list);
	else if(pos >= list->elements)
		return NULL;

	local_node = list->head;
	for(i = 0; (i < pos) && local_node; i++) {
		local_node = local_node->next;
	}

	/* it seems like someone manipulated the list */
	if(!local_node || !local_node->value)
		return NULL;

	return local_node->value;
}

int list_get_head(list_type *list, void * value)
{
	if(!list || !value || !list->head)
		return EINVAL;

	if(!list->head->value) /* it seems like someone manipulated the list */
		return EFAULT;

	memcpy(value, list->head->value, list->element_size);

	return 0;
}

int list_get_tail(list_type *list, void * value)
{
	if(!list || !value || !list->tail)
		return EINVAL;

	if(!list->tail->value) /* it seems like someone manipulated the list */
		return EFAULT;

	memcpy(value, list->tail->value, list->element_size);

	return 0;
}

int list_get(list_type *list, unsigned int pos, void * value)
{
	int i;
	struct list_element *local_node;

	if(!list || !value)
		return EINVAL;

	if(pos == 0) /* before the first */
		return list_get_head(list, value);
	else if((pos + 1) == list->elements) /* the last */
		return list_get_tail(list, value);
	else if(pos >= list->elements)
		return EINVAL;

	local_node = list->head;
	for(i = 0; (i < pos) && local_node; i++) {
		local_node = local_node->next;
	}

	/* it seems like someone manipulated the list */
	if(!local_node || !local_node->value)
		return EFAULT;

	memcpy(value, local_node->value, list->element_size);
	return 0;
}

int list_remove_head(list_type *list)
{
	struct list_element *le, *len;

	if(!list || !list->head)
		return EINVAL;

	le = list->head;

	len = le->next;
	if(!len) {
		list->head = NULL;
		list->tail = NULL;
		list->elements = 0;
	} else {
		list->head = len;
		len->prev = NULL;
		list->elements--;
	}

	list_element_destroy(le);
	return 0;
}

int list_remove_tail(list_type *list)
{
	struct list_element *le, *lep;

	if(!list || !list->tail)
		return EINVAL;

	le = list->tail;

	lep = le->prev;
	if(!lep) {
		list->head = NULL;
		list->tail = NULL;
		list->elements = 0;
	} else {
		list->tail = lep;
		lep->next = NULL;
		list->elements--;
	}

	list_element_destroy(le);
	return 0;

}

int list_remove(list_type *list, int pos)
{
	struct list_element * local_node;
	unsigned int i;

	if(!list || !list->head)
		return EINVAL;

	if(pos >= list->elements)
		return EINVAL;

	if(pos == 0) /* before the first */
		return list_remove_head(list);
	else if((pos + 1) == list->elements) /* after the last */
		return list_remove_tail(list);

	local_node = list->head;
	for(i = 0; (i < pos) && local_node; i++) {
		local_node = local_node->next;
	}

	if(!local_node) /* it seems like someone manipulated the list */
		return EFAULT;

	local_node->next->prev = local_node->prev;
	local_node->prev->next = local_node->next;
	list_element_destroy(local_node);

	list->elements--;

	return 0;
}

int list_is_sorted(list_type *list)
{
	void * max, *cur;
	unsigned int i;
	int comp;

	if(!list || !list->compare)
		return 0;

	if(list->elements < 1)
		return 1;

	/* TODO: this is very inefficient */
	max = list_element(list, 0);

	for (i = 1; i < list->elements; i++) {
		cur = list_element(list, i);
		comp = list->compare(cur, max, list->element_size);

		if(comp > 0)
			max = cur;
		else if(comp < 0)
			return 0;
	}

	return 1;
}
