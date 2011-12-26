#include "stack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

int stack_create(stack_type **stack, size_t element_size)
{
	int rc;
	stack_type *new_s;

	if(!stack || !element_size)
		return EINVAL;

	new_s = (stack_type *) malloc(sizeof(*new_s));
	if(!new_s)
		return ENOMEM;
	memset(new_s, 0, sizeof(*new_s));

	rc = vector_create(&new_s->svector, 0, element_size);
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
	if(!stack)
		return;

	vector_destroy(stack->svector);
	free(stack);
}

void stack_clear(stack_type *stack)
{
	if(!stack || !stack->svector)
		return;

	stack->svector->elements = 0;
}

int stack_push(stack_type *stack, void * value)
{
	if(!stack || !value)
		return EINVAL;

	return vector_add_value(stack->svector, value);
}

int stack_pop(stack_type *stack, void * value)
{
	if(!stack || !stack->svector || !value)
		return EINVAL;

	vector_copy_value(stack->svector, stack->svector->elements - 1, value);
	return vector_del_value(stack->svector, stack->svector->elements - 1);
}

void * stack_top(stack_type *stack)
{
	if(!stack || !stack->svector)
		return NULL;

	return vector_get_value(stack->svector, stack->svector->elements - 1);
}

size_t stack_size(stack_type *stack)
{
	if(!stack || !stack->svector)
		return 0;

	return stack->svector->elements;
}
