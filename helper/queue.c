#include "vector.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <asm/errno.h>

int queue_create(queue_t **queue, size_t len, size_t element_size)
{
	int rc;
	queue_t *new_q;

	new_q = (queue_t *) malloc(sizeof(*new_q));
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

void queue_destroy(queue_t *queue)
{
	vector_destroy(queue->memory);
	free(queue);
}

void queue_clear(queue_t *queue)
{
	queue->memory->elements = 0;
}

int queue_enqueue(queue_t *queue, void * value)
{
	return vector_add_value(queue->memory, value);
}

void queue_dequeue(queue_t *queue, void * value)
{
	vector_copy_value(queue->memory, 0, value);
	vector_del_value(queue->memory, 0);
}

void * queue_head(queue_t *queue)
{
	return vector_get_value(queue->memory, 0);
}

void * queue_tail(queue_t *queue)
{
	return vector_get_value(queue->memory, queue->memory->elements - 1);
}

size_t queue_size(queue_t *queue)
{
	return queue->memory->elements;
}
