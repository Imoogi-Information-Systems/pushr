#ifndef _SINGLETON_H_
#define _SINGLETON_H_

#include <unistd.h>
#include <sys/types.h>

/*
 * A set of function to ensure that only one copy of our program works at a 
 * time. It creates a lock file to signal "busy". Validation is done to make
 * sure the locking process is still active
 */

extern pid_t
singleton_get_lock();

extern void
singleton_set_lock();

extern int
singleton_validate_lock(pid_t locker);

extern void
singleton_override_lock();

extern void
singleton_remove_lock();

extern int
singleton_check();

#endif