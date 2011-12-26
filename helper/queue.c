#include "list.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

int queue_create(queue_type **queue, size_t element_size)
{
	int rc;
	queue_type *new_q;

	if(!queue || !element_size)
		return EINVAL;

	new_q = (queue_type *) malloc(sizeof(*new_q));
	if(!new_q)
		return ENOMEM;
	memset(new_q, 0, sizeof(*new_q));

	rc = list_create(&new_q->qlist, element_size);
	if(rc)
		goto err_free;

	*queue = new_q;

	return 0;
err_free:
	free(new_q);
	return rc;
}

void queue_destroy(queue_type *queue)
{
	if(!queue)
		return;

	list_destroy(queue->qlist);
	free(queue);
}

void queue_clear(queue_type *queue)
{
	int rc;

	if(!queue)
		return;

	do {
		rc = list_remove_head(queue->qlist);
	} while(!rc);
}

int queue_enqueue(queue_type *queue, void * value)
{
	if(!queue || !value)
		return EINVAL;

	return list_append(queue->qlist, value);
}

int queue_dequeue(queue_type *queue, void * value)
{
	int rc;

	if(!queue || !value)
		return EINVAL;

	rc = list_get_head(queue->qlist, value);
	if(rc)
		return rc;

	return list_remove_head(queue->qlist);
}

void * queue_head(queue_type *queue)
{
	if(!queue)
		return NULL;

	return list_head(queue->qlist);
}

void * queue_tail(queue_type *queue)
{
	if(!queue)
		return NULL;

	return list_tail(queue->qlist);
}

size_t queue_size(queue_type *queue)
{
	if(!queue || !queue->qlist)
		return 0;

	return queue->qlist->elements;
}
