#include "settings.h"
#include "pushr.h"
#include "stringbuffer.h"
#include "stringlist.h"
#include "singleton.h"

#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <mysql/mysql.h>
#include <dirent.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int
main(int argc, char *argv[], char *env[])
{
	pid_t child;

	child = fork(); /* We want the main process to return as soon as possible
					 * to prevent the invoker [script, other program] to hang.
					 * The actual work is being done in the child process. */	 

	if (child == -1) {
		output("Couldn't fork worker...");
		return EXIT_FAILURE;
	}

	if (child == 0) {
		/* This is the forked worker */
		
		/*
		 * Singleton checking - making sure only one pushr process is running at a time 
		 */
		int isLocked = singleton_check ();
		CURL *curl;
		
		sleep(5);

		if (isLocked == 1) {
			/* The lock is still valid, quit */
			output("Another pushr is currently active...");
			exit(EXIT_SUCCESS);
		} 

        curl_global_init(CURL_GLOBAL_DEFAULT);
        
        curl = curl_easy_init();
        if (curl) {
			struct curl_slist *headers = NULL; /* customized headers */
			StringBuffer *authorization = string_buffer_create (50); /* the authorization string */

			string_buffer_append (authorization, FIELD_AUTHORIZATION);
			string_buffer_append (authorization, API_KEY);
			
            headers = curl_slist_append(headers, REQUEST_CONTENT_TYPE);
            headers = curl_slist_append(headers, string_buffer_get_string (authorization));

			/* Use command-line arguments to decide whether to read the queue 
			 * from the file system or a MySQL table. Default: file system
			 */
			if (USE_MYSQL && argc == 2 && strcmp(argv[1], "mysql") == 0) {
				/* Use database */
				output("Using MySQL table.");
				handle_database_queue (curl, headers);
			} else {
				if (USE_FILES) {
					/* Use the file system */
					output("Using filesystem");
					handle_file_queue (curl, headers);
				} else {
					output("Error: Filesystem was requested, but it is not enabled.");
				}
			}
                
            /* Free the custom headers */
            curl_slist_free_all(headers);
            /* ... And clean up... */
            curl_easy_cleanup(curl);
                
        }
        
        /* And global clean up */
        curl_global_cleanup();

		/* Remove the lock */
		singleton_remove_lock ();
		
		/* And exit the child process. Good bye! */
		exit(EXIT_SUCCESS);
	} else {
		/* This is the parent. Do nothing here... */
		puts("pushr has been invoked...\n"); /* In case someone is looking for 
											  * some sort of indication... */ 
	}
        
    /* Bye bye! */
    return EXIT_SUCCESS;
}

/*
 * This function is in charge of handling the data from the server. It is being
 * invoked by the cURL library.
 * @param userp is the pointer to the file descriptor
 */
size_t 
receive_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
	size_t real_size = size * nmemb;
	size_t i; /* for loop */
	int c; /* character */
	int insideTag = 0; /* remember if we are inside an HTML tag */
	StringBuffer *string = string_buffer_create (50); /* hold the response */
	char *response;
	int success = 0; /* Did we succeed? */
	int fails = 0; /* Counters */
	int successes = 0;
	
	MessageId *message_data = (MessageId *)userp;

	for (i = 0; i < real_size; i++) 
	{
		c = ((char *)buffer)[i];
		if (c == '<') {
			insideTag = 1;
		} else if (c == '>') {
			insideTag = 0;
		} else {
			if (insideTag == 0) {
				/* Actual data */
				string_buffer_append_char (string, c);
			}
		}
	}

	
	response = string_buffer_get_string (string);
	
	if (strcmp(response, "Error 400 (Bad Request)!!1") == 0)
	{
		/* Bad request error */
		success = 0;
	} else {
		/* Analyze response JSON */
		json_dirty_parse(response, &successes, &fails);
		output("Successes: %d, Fails: %d", successes, fails);
		/* If there are more successful messages than failed, we mark it as a
		 * successful job */
		if (fails > successes) {
			success = 0;
		} else {
			success = 1;
		}
	}

	if (message_data->file_desc != -1) {
		/* We got the data from a file. We need to close it now */
		close(message_data->file_desc); 
	}
	
	if (message_data->file_name != NULL) {
		/* The data came from a file, move it to the appropriate directory and 
		 * write the server response to a file
		 */
		StringBuffer *old_path = string_buffer_create (100);
		StringBuffer *new_path = string_buffer_create (100);
		StringBuffer *res_file = string_buffer_create (100);
		int res_file_desc;

		string_buffer_append (old_path, PATH_MSGS_QUEUE);
		string_buffer_append_char (old_path, '/');
		string_buffer_append (old_path, message_data->file_name);

		if (success) {
			string_buffer_append (new_path, PATH_MSGS_SENT);
			string_buffer_append (res_file, PATH_MSGS_SENT);
		} else {
			string_buffer_append (new_path, PATH_MSGS_ERROR);
			string_buffer_append (res_file, PATH_MSGS_ERROR);
		}
		string_buffer_append_char (new_path, '/');
		string_buffer_append (new_path, message_data->file_name);
		string_buffer_append_char (res_file, '/');
		string_buffer_append (res_file, message_data->file_name);
		string_buffer_append (res_file, ".response");

		if (rename(string_buffer_get_string (old_path), string_buffer_get_string (new_path)) == -1) {
			output("Couldn't move file %s to its new location(%s)", 
			       string_buffer_get_string (old_path), string_buffer_get_string (new_path));
		}

		/* Write the server response to a file
		 * Create the file and give the user and group read and write and only read permission to others
		 */
		res_file_desc = creat(string_buffer_get_string (res_file), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
		if (res_file_desc != -1) {
			/* write data to file */
			write(res_file_desc, response, strlen(response));
			close(res_file_desc);
		} else {
			output("Couldn't write response file (%s).", string_buffer_get_string (res_file));
		}
		
		
		/* Free resources */
		string_buffer_free (old_path);
		string_buffer_free (new_path);
		string_buffer_free (res_file);
		/* Free the file name resource */
		free(message_data->file_name);
	} else {
		/* Data came from database, update it now.
		 * The connction pointer comes from the message_data structure. We are
		 * already connected and we shouldn't close the connection.
		 */
		char *isSent = success ? "1" : "0";
		char *isError = success ? "0" : "1";
		char *update_query = NULL;
		char *escaped_response = (char *)malloc(strlen(response) * 2 + 1);
		char id_string[22]; /* Used to convert the message id from long to string */
		
		if (escaped_response == NULL) {
			/* Error allocation memory, use an empty string */
			escaped_response = "";
		} else {
			(void) mysql_real_escape_string(message_data->mysql_connection, escaped_response, response, strlen(response));
		}

		/* The query creation function takes only strings as arguments. We first
		 * convert the id from long to string and then we create the query string */
		sprintf(id_string, "%ld", message_data->id);
		update_query = db_create_query("UPDATE PushrMessages SET IsSent = %s, "
		                               "IsError = %s, Timestamp = NOW(), ServerResponse = '%s' "
		                               "WHERE Id = %s",
		                               4, isSent, isError, escaped_response, id_string);

		if (update_query != NULL) {
			string_list_push (list, update_query);
			/* Since the list copies the data, we need to free the source as it
			 * is no longer needed */
			free(update_query);
			if (strlen(escaped_response) > 0) {
				/* Not an empty string, we need to free it */
				free(escaped_response);
			}
		} else {
			/* Error allocating memory to the update query */
			output("Error allocating memory for update query");
		}
	}
	
	free(message_data); /* And free the resources */
	string_buffer_free (string);
	return real_size; /* Return the number of chars handled. */
}

/*
 * This function is in charge of producing the data to be sent to the server.
 * It is being used by the cURL library which supplies the parameters.
 * As per the settings, userp is a pointer to the string to be sent.
 * @return the number of chars written
 */
size_t
send_data(char *bufptr, size_t size, size_t nitems, void *userp)
{
	size_t real_size = size * nitems;
	char *message = (char *)userp;
	size_t i; /* for the loop */
	size_t message_length = strlen(message);
	static size_t written = 0; /* counter for the message */
	

	for (i = 0; i < real_size; i++)
	{
		if (written < message_length) 
		{
			bufptr[i] = message[written];
			written++;
		} 
		else
		{
			break;
		}
	}

	
	return i; /* Return the number of characters written */
}

/*
 * Outputs messages.
 * Since pushr is intended as a service of sorts, all output is directed to the
 * stderr.
 * @param message the message to output
 */
void
output(char* format,...) 
{
	va_list args;
	fprintf(stderr, "pushr: ");
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
}

void
send_message(CURL *curl, struct curl_slist *headers, char *message, MessageId *data) 
{
	CURLcode response;
	
	/* Set the requested url to the one of the push service */
    curl_easy_setopt(curl, CURLOPT_URL, PUSH_POST_URL);
    /* Allow following redirections */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    /* Set our receive_data function to handle the data sent FROM the server */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, receive_data); 
	/* Send the file description to the data receiver function */
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)data);
    /* Set our send_data function to handle the data sent TO the server */
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, send_data);
	/* Send the message to the writer function */
	curl_easy_setopt(curl, CURLOPT_READDATA, message);
    /* Set the request to be HTTP POST */
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
	/* Set content length based on the message's length */
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(message));
    /* Set the request heasers as per Google's requirments */
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	
	/* For debugging purposes: */
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
                
    /* Perform the request */
    response = curl_easy_perform(curl);
    if (response != CURLE_OK) {
		/* An error has occured */
        output("cURL Error: %s", curl_easy_strerror(response));
		/* We are calling the receive_data function to handle this response
		 * as well, so we can mark this job as failed */
		receive_data(NULL, 0, 0, (void *)data);
    }
}

void
handle_file_queue(CURL *curl, struct curl_slist *headers)
{
	DIR *d = opendir(PATH_MSGS_QUEUE);
	struct dirent *entry;
	StringBuffer *message = string_buffer_create (250);
	StringBuffer *full_path = string_buffer_create (100);
	
	if (d == NULL) 
	{
		output("Error opening message queue directory");
		return;
	}

	while ((entry = readdir(d)) != NULL)
	{
		/* Handle file */
		int file_desc = -1;
		MessageId *message_data;
		
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue; /* Not actual files... */
		}

		string_buffer_append (full_path, PATH_MSGS_QUEUE);
		string_buffer_append_char (full_path, '/');
		string_buffer_append (full_path, entry->d_name);
		if ((file_desc = open(string_buffer_get_string (full_path), O_RDONLY)) == -1) {
			output("Error opening file: %s", string_buffer_get_string (full_path));
			string_buffer_recycle (full_path);
			continue;
		}
		string_buffer_recycle (full_path);
		
		message_data = (MessageId *)malloc(sizeof(MessageId));
		if (message_data == NULL) {
			output("Error creating message data construct. Out of memeory?");
			continue;
		}

		message_data->file_desc = file_desc;
		message_data->file_name = (char *)malloc(strlen(entry->d_name) + 1);
		if (message_data->file_name == NULL) {
			
			output("Error creating part of the message data construct. Out of memory?");
			free(message_data);
			continue;
		}
		(void) strcpy(message_data->file_name, entry->d_name); /* strcpy takes care of null-terminating the string */

		build_message_from_file (file_desc, message);

		if (message->length > 0) {
			send_message (curl, headers, string_buffer_get_string (message), message_data);
		} else {
			/* Error parsing message, move to error directory */
			StringBuffer *old_path = string_buffer_create (100);
			StringBuffer *new_path = string_buffer_create (100);

			string_buffer_append (old_path, PATH_MSGS_QUEUE);
			string_buffer_append (old_path, message_data->file_name);
			string_buffer_append (new_path, PATH_MSGS_ERROR);
			string_buffer_append (new_path, message_data->file_name);

			if (rename(string_buffer_get_string (old_path), string_buffer_get_string (new_path)) == -1)
			{
				output("Couldn't move file %s to its new location(%s)", 
			   	 string_buffer_get_string (old_path), string_buffer_get_string (new_path));
			}

			string_buffer_free (old_path);
			string_buffer_free (new_path);
		}
		string_buffer_recycle (message);
	}
	
	closedir(d);

	string_buffer_free (message);
	string_buffer_free (full_path);
}

void
build_message_from_file(int file, StringBuffer *buffer)
{
	FILE *message = fdopen(file, "r");
	int in; /* character input */
	int has_more_characters = 0; /* a flag to designate that we broke the loop */
	
	if (message == NULL) {
		output("Error opening file");
		return;
	}

	/* Build the message JSON */
	string_buffer_append_char (buffer, '{');
	
	/* Registration ids (String Array) */
	string_buffer_append (buffer, FIELD_REGISTRATION_IDS);
	string_buffer_append (buffer, " : [");
	while ((in = fgetc(message)) != EOF) {
		/* Detect when the line ends */
		if (in == '\n') {
			/* No more data for this field, break and set flag */
			has_more_characters = 1;
			break;
		} else {
			/* Append character to the message buffer */
			string_buffer_append_char (buffer, in);
		}
	}
	string_buffer_append_char (buffer, ']');

	/* Notification key (String) */
	if (has_more_characters) {
		string_buffer_append_char (buffer, ','); /* Field separation */
		string_buffer_append (buffer, FIELD_NOTIFICATION_KEY);
		string_buffer_append (buffer, ": \"");
		has_more_characters = 0; /* Reset flag */
		while ((in = fgetc(message)) != EOF) {
			/* Detect when the line ends */
			if (in == '\n') {
				/* No more data for this field, break and set flag */
				has_more_characters = 1;
				break;
			} else {
				/* Append character to the message buffer */
				string_buffer_append_char (buffer, in);
			}
		}
		string_buffer_append_char (buffer, '\"');
	}
	
	/* Data (JSON Object) */
	if (has_more_characters) {
		string_buffer_append_char (buffer, ','); /* Field separation */
		string_buffer_append (buffer, FIELD_DATA);
		string_buffer_append (buffer, " : ");
		has_more_characters = 0; /* Reset flag */
		while ((in = fgetc(message)) != EOF) {
			/* Detect when the line ends */
			if (in == '\n') {
				/* No more data for this field, break and set flag */
				has_more_characters = 1;
				break;
			} else {
				/* Append character to the message buffer */
				string_buffer_append_char (buffer, in);
			}
		}
	}
	
	/* Collapse key (String) */
	if (has_more_characters) {
		string_buffer_append_char (buffer, ','); /* Field separation */
		string_buffer_append (buffer, FIELD_COLLAPSE_KEY);
		string_buffer_append (buffer, " : \"");
		has_more_characters = 0; /* Reset flag */
		while ((in = fgetc(message)) != EOF) {
			/* Detect when the line ends */
			if (in == '\n') {
				/* No more data for this field, break and set flag */
				has_more_characters = 1;
				break;
			} else {
				/* Append character to the message buffer */
				string_buffer_append_char (buffer, in);
			}
		}
		string_buffer_append_char (buffer, '\"');
	}
	
	/* Idle delay (Boolean) */
	if (has_more_characters) {
		string_buffer_append_char (buffer, ','); /* Field separation */
		string_buffer_append (buffer, FIELD_DELAY_WHILE_IDLE);
		string_buffer_append (buffer, " : ");
		has_more_characters = 0; /* Reset flag */
		while ((in = fgetc(message)) != EOF) {
			/* Detect when the line ends */
			if (in == '\n') {
				/* No more data for this field, break and set flag */
				has_more_characters = 1;
				break;
			} else {
				/* Append character to the message buffer */
				string_buffer_append_char (buffer, in);
			}
		}
	}
	
	/* Time to live (Number) */
	if (has_more_characters) {
		string_buffer_append_char (buffer, ','); /* Field separation */
		string_buffer_append (buffer, FIELD_TTL);
		string_buffer_append (buffer, " : ");
		has_more_characters = 0; /* Reset flag */
		while ((in = fgetc(message)) != EOF) {
			/* Detect when the line ends */
			if (in == '\n') {
				/* No more data for this field, break and set flag */
				has_more_characters = 1;
				break;
			} else {
				/* Append character to the message buffer */
				string_buffer_append_char (buffer, in);
			}
		}
	}
	
	/* Package name (String) */
	if (has_more_characters) {
		string_buffer_append_char (buffer, ','); /* Field separation */
		string_buffer_append (buffer, FIELD_RESTRICTED_PACKAGE);
		string_buffer_append (buffer, " : \"");
		has_more_characters = 0; /* Reset flag */
		while ((in = fgetc(message)) != EOF) {
			/* Detect when the line ends */
			if (in == '\n') {
				/* No more data for this field, break and set flag */
				has_more_characters = 1;
				break;
			} else {
				/* Append character to the message buffer */
				string_buffer_append_char (buffer, in);
			}
		}
		string_buffer_append_char (buffer, '\"');
	}
	
	/* Dry run (Boolean) */
	if (has_more_characters) {
		string_buffer_append_char (buffer, ','); /* Field separation */
		string_buffer_append (buffer, FIELD_DRY_RUN);
		string_buffer_append (buffer, " : ");
		has_more_characters = 0; /* Reset flag */
		while ((in = fgetc(message)) != EOF) {
			/* Detect when the line ends */
			if (in == '\n') {
				/* No more data for this field, break and set flag */
				has_more_characters = 1;
				break;
			} else {
				/* Append character to the message buffer */
				string_buffer_append_char (buffer, in);
			}
		}
	}
	
	string_buffer_append_char (buffer, '}');
	
	/* The file will be closed after we send the message to the server */
}

void
handle_database_queue(CURL *curl, struct curl_slist *headers)
{
	MYSQL *conn;
	MYSQL_RES* results;
	MYSQL_ROW row;
	StringBuffer *message = string_buffer_create (250);
	char *update_command = NULL; /* For list iteration */
	int result_count = 1; /* Used in a loop */

	/* Create StringList (to hold update commands) */
	list = string_list_create ();
	
	/* Open database */
	conn = mysql_init(NULL);

	if (! mysql_real_connect(conn, MYSQL_SERVER, MYSQL_USER, MYSQL_PASSWORD, MYSQL_SCHEMA, 0, NULL, CLIENT_MULTI_STATEMENTS)) {
		output("Database connection error: %s", mysql_error(conn));
		return;
	}

	mysql_set_server_option(conn, MYSQL_OPTION_MULTI_STATEMENTS_ON);

	mysql_set_character_set(conn, "utf8mb4");

	/*
	 * To catch messages and send them without significant delay, keep quering the 
	 * server, till zero messages are returned. Then we can quit.
	 */
	while (result_count > 0) {
		/* Query messages tables for messages not sent and not marked as errors */
		if (mysql_query(conn, "SELECT Id, RegistrationIds, NotificationKey, Data, CollapseKey, DelayWhileIdle, "
			            "TimeToLive, PackageName, DryRun FROM PushrMessages WHERE IsSent = 0 AND IsError = 0")) {
			/* Error */
			output("Database query error: %s", mysql_error(conn));
			/*mysql_close(conn);
			return;*/
			break;
		}

		results = mysql_use_result(conn);
		result_count = 0; /* Reset counter for each query */

		if (results == NULL) {
			output("Command returned null.");
			mysql_free_result(results);
			/*mysql_close(conn);
			return;*/
			break;
		}

		if (mysql_num_fields(results) == 0) {
			output("No results for command.");
			mysql_free_result(results);
			/*mysql_close(conn);
			return;*/
			break;
		}

		while ((row = mysql_fetch_row(results)) != NULL) {
			build_message_from_row (&row, message);
			if (message->length > 0) {
				MessageId *message_data = (MessageId *)malloc(sizeof(MessageId));
				if (message_data == NULL) {
					output("Error creating message data construct. Out of memeory?");
					continue;
				}
				message_data->id = strtol(row[0], NULL, 10);
				message_data->mysql_connection = conn;
				message_data->file_name = NULL; /* It is not a file */
				send_message (curl, headers, string_buffer_get_string (message), message_data);
				string_buffer_recycle (message);
			}
			result_count++;
		}
	
		/* Free result resource */
		mysql_free_result(results);

		/* Now use the update command list to update the database */
		update_command = string_list_get_next (list);
		while (update_command != NULL) {
			if (mysql_query(conn, update_command)) {
				/* Error */
				output("Database update query error: %s (%s)", mysql_error(conn), update_command);
			}
			update_command = string_list_get_next (list);
		}
		/* Recycle the list for the next potential loop */
		string_list_recycle (list);
	}
	
	/* Close database connection */
	mysql_close(conn);

	/* Free the StringList */
	string_list_free (list);
}

void
build_message_from_row(MYSQL_ROW *row, StringBuffer *buffer)
{
	/* Build the message JSON */
	string_buffer_append_char (buffer, '{');
	
	/* Registration ids (String Array) */
	string_buffer_append (buffer, FIELD_REGISTRATION_IDS);
	string_buffer_append (buffer, " : [");
	string_buffer_append (buffer, (*row)[1]);
	string_buffer_append_char (buffer, ']');

	/* Notification key (String) */
	if ((*row)[2] != NULL) {
		string_buffer_append_char (buffer, ','); /* Field separation */
		string_buffer_append (buffer, FIELD_NOTIFICATION_KEY);
		string_buffer_append (buffer, ": \"");
		string_buffer_append (buffer, (*row)[2]);
		string_buffer_append_char (buffer, '\"');
	}
	
	/* Data (JSON Object) */
	if ((*row)[2] != NULL && strlen((*row)[2]) > 0) { /* If not null or empty string */
		string_buffer_append_char (buffer, ','); /* Field separation */
		string_buffer_append (buffer, FIELD_DATA);
		string_buffer_append (buffer, " : ");
		string_buffer_append (buffer, (*row)[3]);
	}
	
	/* Collapse key (String) */
	if ((*row)[4] != NULL) {
		string_buffer_append_char (buffer, ','); /* Field separation */
		string_buffer_append (buffer, FIELD_COLLAPSE_KEY);
		string_buffer_append (buffer, " : \"");
		string_buffer_append (buffer, (*row)[4]);
		string_buffer_append_char (buffer, '\"');
	}
	
	/* Idle delay (Boolean) */
	if ((*row)[5] != NULL) {
		char *value = (*row)[5];
		string_buffer_append_char (buffer, ','); /* Field separation */
		string_buffer_append (buffer, FIELD_DELAY_WHILE_IDLE);
		string_buffer_append (buffer, " : ");
		if (strcmp("1", value) == 0) {
			string_buffer_append (buffer, "true"); 
		} else {
			string_buffer_append (buffer, "false");
		}
	}
	
	/* Time to live (Number) */
	if ((*row)[6] != NULL && strcmp("0", (*row)[6]) != 0) { /* Not null and not the default 0 */
		string_buffer_append_char (buffer, ','); /* Field separation */
		string_buffer_append (buffer, FIELD_TTL);
		string_buffer_append (buffer, " : ");
		string_buffer_append (buffer, (*row)[6]);
	}
	
	/* Package name (String) */
	if ((*row)[7] != NULL) {
		string_buffer_append_char (buffer, ','); /* Field separation */
		string_buffer_append (buffer, FIELD_RESTRICTED_PACKAGE);
		string_buffer_append (buffer, " : \"");
		string_buffer_append (buffer, (*row)[7]);
		string_buffer_append_char (buffer, '\"');
	}
	
	/* Dry run (Boolean) */
	if ((*row)[2] != NULL) {
		char *value = (*row)[8];
		string_buffer_append_char (buffer, ','); /* Field separation */
		string_buffer_append (buffer, FIELD_DRY_RUN);
		string_buffer_append (buffer, " : ");
		if (strcmp("1", value) == 0) {
			string_buffer_append (buffer, "true"); 
		} else {
			string_buffer_append (buffer, "false");
		}
	}
	
	string_buffer_append_char (buffer, '}');
}

/*
 * Helper function to dynamically create the query string.
 * Don't forget to free the string when you are done.
 * Important: Parameters can be only strings (for now) !!!
 * @param template the printf-style template
 * @param count the number of arguments to follow
 * @param ... the arguments for the template
 * @return a pointer to a malloc'd string containing the query
 */
char *
db_create_query(char *template, int count, ...) {
	va_list args, args2;
	int length;
	char *query = NULL;
	int i = 0;

	va_start(args, count);
	va_copy(args2, args);
	length = strlen(template);

	for (; i < count; i++) {
		char *str = va_arg(args, char *);
		length += strlen(str);
	}
	va_end(args);

	query = (char *) malloc(length + 1);
	if (query == NULL) {
		/* Error - not enough memory? */
		output("Creating query error - not enough memory");
		return NULL;
	} 

	(void) vsprintf(query, template, args2);

	va_end(args2);

	return query;
}

/* Dirty JSON parse function. The server returns a JSON. We need to parse it to
 * determine how many successes we had vs. how many fails.
 * @param json_string the JSON string to parse
 * @param successes pointer to an int that will hold the nunber of successful sends
 * @param fails pointer to an int that will hold the number of fails sends.
 */
void
json_dirty_parse(char *json_string, int *successes, int *fails)
{
	const char *success_field = "\"success\":";
	const char *failure_field = ",\"failure\":";

	char *pos = strstr(json_string, success_field);
	
	if (pos == NULL) {
		/* The success was not found in the JSON string.
		 * We don't know how to handle this, so set it as an error */
		*successes = 0;
		*fails = 1;
	} else {
		/* Advance the cursor to the start of the number. */
		pos += strlen(success_field);
		*successes = atoi(pos);
	}

	pos = strstr(json_string, failure_field);
	
	if (pos == NULL) {
		/* The field is missing. This is not the JSON string we are used to so
		 * we'll mark it as an error */
		*successes = 0;
		*fails = 1;
	} else {
		/* Advance the cursor to the start of the number */
		pos += strlen(failure_field);
		*fails = atoi(pos);
	}
}
