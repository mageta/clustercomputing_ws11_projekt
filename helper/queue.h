#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>

#include "vector.h"

/*
 * TODO:
 *	change this quick vector-implementation into a list-implementation
 */

struct queue {
	vector_type * memory;
};

typedef struct queue queue_type;

int queue_create(queue_type **queue, size_t len, size_t element_size);
void queue_destroy(queue_type *queue);
void queue_clear(queue_type *queue);

int queue_enqueue(queue_type *queue, void * value);
void queue_dequeue(queue_type *queue, void * value);
void * queue_head(queue_type *queue);
void * queue_tail(queue_type *queue);
size_t queue_size(queue_type *queue);

#endif //QUEUE_H
