#ifndef _PUSHR_H_
#define _PUSHR_H_

#define FIELD_REGISTRATION_IDS "\"registration_ids\""
#define FIELD_NOTIFICATION_KEY "\"notification_key\""
#define FIELD_COLLAPSE_KEY "\"collapse_key\""
#define FIELD_DATA "\"data\""
#define FIELD_DELAY_WHILE_IDLE "\"delay_while_idle\""
#define FIELD_TTL "\"time_to_live\""
#define FIELD_RESTRICTED_PACKAGE "\"restricted_package_name\""
#define FIELD_DRY_RUN "\"dry_run\""
#define FIELD_AUTHORIZATION "Authorization: key="

#define PUSH_POST_URL "https://android.googleapis.com/gcm/send"
#define REQUEST_CONTENT_TYPE "Content-Type:application/json"

#include <curl/curl.h>
#include <mysql/mysql.h>
#include <stdarg.h>

#include "stringbuffer.h"
#include "stringlist.h"

typedef struct {
	int file_desc;
	char *file_name;
	unsigned long id;
	MYSQL *mysql_connection;
} MessageId;

void
output(char* format, ...);

void
send_message(CURL *curl, struct curl_slist *headers, char *message, MessageId *message_data);

size_t 
receive_data(void *buffer, size_t size, size_t nmemb, void *userp); 

size_t
send_data(char *bufptr, size_t size, size_t nitems, void *userp); 

void
handle_file_queue(CURL *curl, struct curl_slist *headers);

void
build_message_from_file(int file, StringBuffer *buffer);

void
handle_database_queue(CURL *curl, struct curl_slist *headers);

void
build_message_from_row(MYSQL_ROW *row, StringBuffer *buffer);

/* DB Helper function */
char *
db_create_query(char *template, int count, ...);

/* Dirty JSON parse function */
void
json_dirty_parse(char *json_string, int *successes, int *fails);

/* Global variables : */
StringList *list = NULL; /* StringList to hold MySQL update commands */

#endif
