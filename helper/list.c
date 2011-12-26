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

int list_create(list_t ** l, size_t element_size)
{
	list_t * list;

	if(!l || !element_size)
		return EINVAL;

	list = (list_t *) malloc(sizeof(*list));
	if(!list)
		return ENOMEM;
	memset(list, 0, sizeof(*list));

	list->head = NULL;
	list->tail = NULL;
	list->elements = 0;
	list->element_size = element_size;

	*l = list;

	return 0;
}

void list_destroy(list_t * list)
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

int list_append(list_t * list, void * value)
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

int list_prepend(list_t * list, void * value)
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

int list_insert_at(list_t * list, unsigned int pos, void * value)
{
	struct list_element * le;
	struct list_element * local_node;
	unsigned int i;

	if(!list || !value)
		return EINVAL;

	if((pos + 1) > list->elements)
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

int list_head(list_t *list, void * value) {
	if(!list || !value || !list->head)
		return EINVAL;

	if(!list->head->value) /* it seems like someone manipulated the list */
		return EFAULT;

	memcpy(value, list->head->value, list->element_size);

	return 0;
}

int list_tail(list_t *list, void * value) {
	if(!list || !value || !list->tail)
		return EINVAL;

	if(!list->tail->value) /* it seems like someone manipulated the list */
		return EFAULT;

	memcpy(value, list->tail->value, list->element_size);

	return 0;
}

int list_get(list_t *list, unsigned int pos, void * value) {
	int i;
	struct list_element *local_node;

	if(!list || !value)
		return EINVAL;

	if(pos == 0) /* before the first */
		return list_head(list, value);
	else if((pos + 1) == list->elements) /* after the last */
		return list_tail(list, value);
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

int list_remove_head(list_t *list) {
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

int list_remove_tail(list_t *list) {
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

int list_remove(list_t *list, int pos) {
	struct list_element * local_node;
	unsigned int i;

	if(!list || !list->head)
		return EINVAL;

	if((pos + 1) > list->elements)
		return EINVAL;

	if(pos == 0) /* before the first */
		return list_remove_head(list);
	else if(pos == list->elements) /* after the last */
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