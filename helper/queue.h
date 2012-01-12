#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>

#include "list.h"

struct queue {
	list_type * qlist;
};

typedef struct queue queue_type;

int queue_create(queue_type **queue, size_t element_size)
	__attribute__((warn_unused_result));
void queue_destroy(queue_type *queue);
void queue_clear(queue_type *queue);

int queue_enqueue(queue_type *queue, void * value)
	__attribute__((warn_unused_result));
int queue_dequeue(queue_type *queue, void * value);
void * queue_head(queue_type *queue)
	__attribute__((warn_unused_result));
void * queue_tail(queue_type *queue)
	__attribute__((warn_unused_result));
size_t queue_size(queue_type *queue);

#endif //QUEUE_H
