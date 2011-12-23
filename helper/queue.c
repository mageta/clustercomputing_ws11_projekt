#include "vector.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

int queue_create(queue_type **queue, size_t len, size_t element_size)
{
	int rc;
	queue_type *new_q;

	new_q = (queue_type *) malloc(sizeof(*new_q));
	if(!new_q)
		return ENOMEM;
	memset(new_q, 0, sizeof(*new_q));

	rc = vector_create(&new_q->memory, len, element_size);
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
	vector_destroy(queue->memory);
	free(queue);
}

void queue_clear(queue_type *queue)
{
	queue->memory->elements = 0;
}

int queue_enqueue(queue_type *queue, void * value)
{
	return vector_add_value(queue->memory, value);
}

void queue_dequeue(queue_type *queue, void * value)
{
	vector_copy_value(queue->memory, 0, value);
	vector_del_value(queue->memory, 0);
}

void * queue_head(queue_type *queue)
{
	return vector_get_value(queue->memory, 0);
}

void * queue_tail(queue_type *queue)
{
	return vector_get_value(queue->memory, queue->memory->elements - 1);
}

size_t queue_size(queue_type *queue)
{
	return queue->memory->elements;
}
