#include "stringbuffer.h"
#include <stdlib.h>
#include <string.h>

/* Create and initialize the StringBuffer
 *  @param length the initial length of the buffer. If value is smaller than 
 * STRING_BUFFER_BUFFER then that value is used.
 * @return a pointer to the newly created StringBuffer, or NULL on error
 */
StringBuffer *
string_buffer_create(int length)
{
	int buffered = STRING_BUFFER_BUFFER;
	StringBuffer *buffer;
	if (length > buffered) 
	{
		buffered = length;
	}

	buffer = (StringBuffer *)malloc(sizeof(StringBuffer));
	if (buffer != NULL) {
		buffer->length = 0;
		buffer->space = buffered;
		buffer->data = (char *)malloc(buffered + 1); /* Allow room for null terminator */
	}

	return buffer;
}

/* Adding text into the buffer
 * @param buffer a pointer to the StringBuffer object
 * @param string the string to append
 * @return the number of characters appended
 */
int
string_buffer_append(StringBuffer *buffer, char *string)
{
	unsigned int in_length = strlen(string); /* Don't count the null-terminator */
	unsigned int i; /* for the loop */
	int written = 0; /* how many characters were appended */

	/* Protect against null pointer exceptions */
	if (string == NULL) {
		return 0;
	}
	
	if (buffer->space - buffer->length < in_length) 
	{
		/* Not enough room for the new string, increase */
		char *alloc = (char *)realloc(buffer->data, buffer->space + STRING_BUFFER_BUFFER + 1); /* Leave room for at least on nul */
		if (alloc == NULL) {
			return 0; /* Error: zero characters appended */
		}
		buffer->space += STRING_BUFFER_BUFFER;
		buffer->data = alloc;
	}

	for (i = 0; i < in_length; i++)
	{
		
		buffer->data[buffer->length] = string[i];
		buffer->length++;
		written++;
		
	}

	return written;
}

/* Adding a single character into the buffer
 * @param buffer a pointer to the StringBuffer we want to add to
 * @param character the character to be added
 * @return whether it was written or not
 */
int
string_buffer_append_char(StringBuffer *buffer, int character)
{
	if (buffer->space - buffer->length < 1) 
	{
		/* Not enough room for the new character, increase */
		char *alloc = (char *)realloc(buffer->data, buffer->space + STRING_BUFFER_BUFFER + 1); /* Allow room for at least on nul */
		if (alloc == NULL) {
			return 0; /* Error: zero characters appended */
		}
		buffer->space += STRING_BUFFER_BUFFER;
		buffer->data = alloc;
	}

	/* Add the character */
	buffer->data[buffer->length] = character;
	buffer->length++;
		

	return 1;
}

/* Return the string buffered
 * @param buffer the pointer to the StringBuffer object
 * @return a pointer to the string, after making sure that it is null-terminated.
 * Call this function to get the string, instead of using the data field of the 
 * StringBuffer. Don't worry about freeing the string - it is done when you call
 * the StringBuffer's free function
 */
char *
string_buffer_get_string(StringBuffer *buffer)
{
	/* Since we have been ignoring the null-terminators so far, we need now
	 * to make sure that our string ends in a null-terminator.
	 */
	unsigned int i; /* for the loop */

	/* The loop goes from the character after the last actual character up to
	 * and include the size of the buffer. It is inclusive because we have saved
	 * when created, one character for the null-terminator. We fill it now.
	 */
	for (i = buffer->length; i <= buffer->space; i++)
	{
		buffer->data[i] = '\0';
	}

	return buffer->data;
}

/* Frees the StringBuffer (and the string created) when done
 * @param buffer pointer to the StringBuffer object
 */
void
string_buffer_free(StringBuffer *buffer)
{
	free(buffer->data);
	free(buffer);
}

/* Recycles the StringBuffer and lets you use it again
 * @param buffer pointer to the StringBuffer object
 */
void
string_buffer_recycle(StringBuffer *buffer)
{
	/* Instead of deleting, set the buffer so it appears as if it contains an
	 * empty string and prepare it for the next append
	 */
	buffer->data[0] = '\0'; /* Will be overwriten on the next append. For now,
							 * an empty string */ 
	buffer->length = 0;
}

