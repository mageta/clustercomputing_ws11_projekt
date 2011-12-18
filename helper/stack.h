#ifndef STACK_H
#define STACK_H

#include <stdio.h>

#include "vector.h"

struct stack {
	vector_t * memory;
};

typedef struct stack stack_t;

int stack_create(stack_t **stack, size_t len, size_t element_size);
void stack_destroy(stack_t *stack);
void stack_clear(stack_t *stack);

int stack_push(stack_t *stack, void * value);
void stack_pop(stack_t *stack, void * value);
void * stack_top(stack_t *stack);
size_t stack_size(stack_t *stack);

#endif //STACK_H
