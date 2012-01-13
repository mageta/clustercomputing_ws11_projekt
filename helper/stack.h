#ifndef STACK_H
#define STACK_H

#include <stdio.h>

#include "vector.h"

struct stack {
	vector_type * svector;
};

typedef struct stack stack_type;

int stack_create(stack_type **stack, size_t element_size)
	__attribute__((warn_unused_result));
void stack_destroy(stack_type *stack);
void stack_clear(stack_type *stack);

int stack_push(stack_type *stack, void * value)
	__attribute__((warn_unused_result));
int stack_pop(stack_type *stack, void * value);
void * stack_top(stack_type *stack)
	__attribute__((warn_unused_result));
size_t stack_size(stack_type *stack);

#endif //STACK_H
