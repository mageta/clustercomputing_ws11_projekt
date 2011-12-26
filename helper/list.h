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

typedef struct list list_type;

int list_create(list_type ** list, size_t element_size);
void list_destroy(list_type * list);

int list_append(list_type * list, void * value);
int list_prepend(list_type * list, void * value);
int list_insert_at(list_type * list, unsigned int pos, void * value);

void * list_head(list_type *list);
void * list_tail(list_type *list);

int list_get_head(list_type *list, void * value);
int list_get_tail(list_type *list, void * value);
int list_get(list_type *list, unsigned int pos, void * value);

int list_remove_head(list_type *list);
int list_remove_tail(list_type *list);
int list_remove(list_type *list, int pos);

#define for_each_list_element(list, i) \
	for((i) = 0; (i) < (list)->elements; (i)++)

#endif // LIST_H
