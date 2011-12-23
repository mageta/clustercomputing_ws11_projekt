#include "stack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

int stack_create(stack_type **stack, size_t len, size_t element_size)
{
	int rc;
	stack_type *new_s;

	new_s = (stack_type *) malloc(sizeof(*new_s));
	if(!new_s)
		return ENOMEM;
	memset(new_s, 0, sizeof(*new_s));

	rc = vector_create(&new_s->memory, len, element_size);
	if(rc)
		goto err_free;

	*stack = new_s;

	return 0;
err_free:
	free(new_s);
	return rc;
}

void stack_destroy(stack_type *stack)
{
	vector_destroy(stack->memory);
	free(stack);
}

void stack_clear(stack_type *stack)
{
	stack->memory->elements = 0;
}

int stack_push(stack_type *stack, void * value)
{
	return vector_add_value(stack->memory, value);
}

void stack_pop(stack_type *stack, void * value)
{
	vector_copy_value(stack->memory, stack->memory->elements - 1, value);
	vector_del_value(stack->memory, stack->memory->elements - 1);
}

void * stack_top(stack_type *stack)
{
	return vector_get_value(stack->memory, stack->memory->elements - 1);
}

size_t stack_size(stack_type *stack)
{
	return stack->memory->elements;
}
