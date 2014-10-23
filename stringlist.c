#include "stringlist.h"
#include <stdlib.h>
#include <string.h>

/*
 * Creates a new (empty) string list
 * @return a pointer to the string list object
 */ 
extern StringList *
string_list_create()
{
	StringList *list = (StringList *)malloc(sizeof (StringList));
	if (list != NULL) {
		list->size = 0;
		list->head = NULL;
		list->write_cursor = NULL;
		list->read_cursor = NULL;
	}

	return list;
}

/*
 * Creates a new StringList node with the string data filled in
 * @param data the string data for this node to hold
 * @return a pointer to the newly created node
 */
static struct StringListNode *
string_list_create_node(char *data)
{
	struct StringListNode *node = (struct StringListNode *)malloc(sizeof(struct StringListNode));
	if (node != NULL) {
		node->data = (char *)malloc(strlen(data) + 1);
		if (node->data != NULL) {
			(void) strcpy(node->data, data);
		}
		node->next = NULL;
	}

	return node;
}

/*
 * Adds a new node containg the data to the end of the StringList list
 * @param list a pointer to the list object
 * @data the string data to hold
 */
extern void
string_list_push(StringList *list, char *data)
{
	struct StringListNode *node = string_list_create_node (data);
	if (node != NULL) {
		if (list->write_cursor == NULL) {
			/* This is the first element in the list */
			list->head = node;
			list->write_cursor = node;
		} else {
			/* Adding to the end of the list. write_cursor points to the currently
			 * last element */
			list->write_cursor->next = node;
			list->write_cursor = node;
		}
		list->size++;
		/* Update the reader cursor if it points to null at the moment */
		if (list->read_cursor == NULL) {
			list->read_cursor = node;
		}
	}
}



/*
 * Returns a pointer to the next string on the StringList list and advances
 * the reader cursor to the next element in the list
 * @param list the pointer to the StringList list
 * @return a pointer to the next string in the list or NULL if we are at the end
 * of the list
 */
extern char *
string_list_get_next(StringList *list)
{
	char *result;
	if (list->read_cursor == NULL) {
		result = NULL;
	} else {
		result = list->read_cursor->data;
		/* Advance the read cursor to the next poisition */
		list->read_cursor = list->read_cursor->next;
	}

	return result;
}

/*
 * Frees the string list and its resources
 * @param list the list to be freed
 */
extern void
string_list_free(StringList *list)
{
	struct StringListNode *node, *next;
	node = list->head;
	while (node != NULL) {
		next = node->next;
		/* First free the data string */
		if (node->data != NULL) {
			free(node->data);
		}
		/* Then free the node itself */
		free(node);
		/* And move to the next one */
		node = next;
	}
	/* Finally, free the list object itself */
	free(list);
}

/*
 * Resets the list to zero items
 * @param list the list to be recycled
 */
extern void
string_list_recycle(StringList *list) 
{
	/* First free all the members of the list */
	struct StringListNode *node, *next;
	node = list->head;
	while (node != NULL) {
		next = node->next;
		/* First free the data string */
		if (node->data != NULL) {
			free(node->data);
		}
		/* Then free the node itself */
		free(node);
		/* And move to the next one */
		node = next;
	}

	/* Set size to zero */
	list->size = 0;
	/* Set cursor pointers */
	list->head = NULL;
	list->read_cursor = NULL;
	list->write_cursor = NULL;
}
