/*
 * A simple list that stores strings.
 * Use push to add a string to the end of the list.
 * Use get next to get a pointer to the next available string and advance the 
 * cursor to the next link
 */

#ifndef _STRINGLIST_H_
#define _STRINGLIST_H_

struct StringListNode {
	char *data;
	struct StringListNode *next;
};

typedef struct {
	struct StringListNode *head;
	struct StringListNode *write_cursor;
	struct StringListNode *read_cursor;
	int size;
} StringList;

extern StringList *
string_list_create();

extern void
string_list_push(StringList *list, char *data);

extern char *
string_list_get_next(StringList *list);

extern void
string_list_free(StringList *list);

extern void
string_list_recycle(StringList *list);

#endif 