#include "singleton.h"
#include "settings.h"
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

/*
 * Reads the lock file and returns the process id of the locking process
 * @return the process id of the locking process
 */
extern pid_t
singleton_get_lock()
{
	/* Open lock file for reading */
	FILE *lock = fopen(LOCK_FILE, "r");
	char buffer[10];
	/* Check file validity */
	if (lock == NULL) {
		if (errno == ENOENT) {
			/* File does not exist */
			return 0;
		}
		
		return -1;
	}
	/* Read the file */
	(void) fgets(buffer, 10, lock);
	/* Close the file */
	fclose (lock);
	/* Convert into number */
	if (strlen(buffer) > 0) {
		return (pid_t) atoi(buffer);
	} else {
		return 0;
	}
}

/*
 * Sets the current process as the locking process
 */
extern void
singleton_set_lock()
{
	/* Get current process id */
	pid_t me = getpid();
	/* Open the lock file for writing */
	FILE *lock = fopen(LOCK_FILE, "w");
	/* Write the current process id to the lock file */
	fprintf (lock, "%d", me);
	/* Close the file */
	fclose(lock);
}

/*
 * Validated the locking process and making sure it is still active
 * @return 1 if the lock is still active, or 0 if the lock needs to be overridden. 
 */
extern int
singleton_validate_lock(pid_t locker)
{
	/* If locker equals zero = no lock */
	if (locker == 0) {
		return 0;
	}
	
	/* See if the process locker is still active */
	if (kill(locker, 0) == 0) {
		/* The process is running */
		return 1;
	} else {
		return 0;
	}	
}

/*
 * Deletes the current lock file and set this running process as the locking process
 */
extern void
singleton_override_lock()
{
	singleton_remove_lock ();
	singleton_set_lock ();
}

/*
 * When done, remove the lock
 */
extern void
singleton_remove_lock()
{
	remove(LOCK_FILE);
}

/*
 * Opens the lock file, reads it's content and write our pid 
 * @return the pid read from the lock file
 */
static pid_t
singleton_read_and_write_lock() 
{
	/* Open lock file for reading and writing */
	FILE *lock = fopen(LOCK_FILE, "r+");
	char buffer[10];
	pid_t read_lock;
	pid_t me = getpid();
	
	/* Check file validity */
	if (lock == NULL) {
		if (errno == ENOENT) {
			/* File does not exist */
			read_lock = 0;
			/* Create the file and open it for writing */
			lock = fopen(LOCK_FILE, "w");
			if (lock == NULL) {
				/* Error opening the file for writing */
				return 0;
			}
		}	
		
	} else {
		/* Read the file */
		(void) fgets(buffer, 10, lock);
		read_lock = (pid_t) atoi(buffer);
	}
	/* Write out lock to the file, but first move the cursor to the start */
	fseek (lock, 0L, SEEK_SET);
	fprintf (lock, "%d%c%c%c%c%c", me, 0, 0, 0, 0, 0); /* Pad with NUL in case there
													 * is a length difference 
													 * between the two pids */
	/* Close the file */
	fclose (lock);

	return read_lock;
}

/*
 * Combine all the calls to see if we can proceed or not
 * @return 0 if we are good to go (not locked) or 1 if we are locked
 */
extern int
singleton_check()
{
	pid_t lock = singleton_read_and_write_lock ();
	pid_t me = getpid();
	
	if (lock == 0 || lock == me) {
		/* We are the locking process, or there was no lock */
		return 0;
	} else {
		if (singleton_validate_lock (lock)) {
			/* The lock is valid */
			/* Write it back  to the file */
			/* Open the lock file for writing */
			FILE *lock_file = fopen(LOCK_FILE, "w");
			/* Write the current process id to the lock file */
			fprintf (lock_file, "%d", lock);
			/* Close the file */
			fclose(lock_file);
			return 1; /* Quit */
		} else {
			/* The lock is invalid */
			/* We took control */
			return 0; /* Proceed */
		}
	}
}


