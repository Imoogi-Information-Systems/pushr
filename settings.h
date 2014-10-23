/*
 * This is the settings file, the only file that you SHOULD edit :)
 */

#ifndef _SETTINGS_H_
#define _SETTINGS_H_

/*
 * The API key to the Google Cloud Messaging service.
 */
#define API_KEY ""

/*
 * If you don't want to use files at all, set the option to 0.
 * If you do, make sure to set the three required directories
 */
#define USE_FILES 1
#define PATH_MSGS_QUEUE "/var/pushr/messages.queue"
#define PATH_MSGS_SENT "/var/pushr/messages.sent"
#define PATH_MSGS_ERROR "/var/pushr/messages.error"

/*
 * If you don't want to use a MySQL database at all, set the option to 0.
 * If you do, make sure to set the required fields below and create the 
 * table (SQL code can be found in the readme file)
 */
#define USE_MYSQL 1
#define MYSQL_SERVER "localhost"
#define MYSQL_USER "pushr_user"
#define MYSQL_PASSWORD "xrNV1WkKVtGTH8ym"
#define MYSQL_SCHEMA "pushr"

/*
 * Only one instance should be active at a time. We use a file called LOCK to 
 * hold the process id of the active pushr to prevent other instances.
 * Fill in the path to the lock file.
 */
#define LOCK_FILE "/var/pushr/.lock"

#endif
