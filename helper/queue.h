#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>

#include "vector.h"

/*
 * TODO:
 *	change this quick vector-implementation into a list-implementation
 */

struct queue {
	vector_t * memory;
};

typedef struct queue queue_t;

int queue_create(queue_t **queue, size_t len, size_t element_size);
void queue_destroy(queue_t *queue);
void queue_clear(queue_t *queue);

int queue_enqueue(queue_t *queue, void * value);
void queue_dequeue(queue_t *queue, void * value);
void * queue_head(queue_t *queue);
void * queue_tail(queue_t *queue);
size_t queue_size(queue_t *queue);

#endif //QUEUE_H
