/*
 * The StringBuffer is used to dynamically create and concate strings 
*/

#ifndef _STRINGBUFFER_H_
#define _STRINGBUFFER_H_

/* The default buffer size */
#define STRING_BUFFER_BUFFER 25

struct StringBuffer {
	char *data; /* the character data */
	unsigned int length; /* the actual length of the string */
	unsigned int space; /* the total length of the buffer */
};

typedef struct StringBuffer StringBuffer;

/* Create and initialize the StringBuffer */
StringBuffer *
string_buffer_create(int length);

/* Adding text into the buffer */
int
string_buffer_append(StringBuffer *buffer, char *string);

/* Adding a single character into the buffer */
int
string_buffer_append_char(StringBuffer *buffer, int character);

/* Return the string buffered */
char *
string_buffer_get_string(StringBuffer *buffer);

/* Frees the StringBuffer when done */
void
string_buffer_free(StringBuffer *buffer);

/* Recycles the StringBuffer and lets you use it again */
void
string_buffer_recycle(StringBuffer *buffer);

#endif