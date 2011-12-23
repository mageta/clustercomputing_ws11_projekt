#ifndef LIST_H
#define LIST_H

#include <stdio.h>
#include <string.h>

struct list {
	struct list_element * head; /* the first element */
	struct list_element * tail; /* teh last element */

	size_t elements;
	size_t element_size;
};

typedef struct list list_t;

int list_create(list_t ** list, size_t element_size);
void list_destroy(list_t * list);

int list_append(list_t * list, void * value);
int list_prepend(list_t * list, void * value);
int list_insert_at(list_t * list, unsigned int pos, void * value);

int list_head(list_t *list, void * value);
int list_tail(list_t *list, void * value);
int list_get(list_t *list, unsigned int pos, void * value);

int list_remove_head(list_t *list);
int list_remove_tail(list_t *list);
int list_remove(list_t *list, int pos);

#endif // LIST_H
