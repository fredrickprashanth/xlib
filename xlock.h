
#ifndef __X_LOCK_H__
#define __X_LOCK_H__

//#define LOCK_DEBUG 1
typedef struct xlock_ {
	int fd;
	int *lock_ptr;
	char name[16];
#ifdef LOCK_DEBUG
	int lock_pid, unlock_pid;
#endif
} xlock_t;

xlock_t * xlock_open(char *name);
xlock_t * xlock_create(char *name);
int xlock_close(xlock_t *);
int xlock_lock(xlock_t *);
int xlock_unlock(xlock_t *);

#endif
