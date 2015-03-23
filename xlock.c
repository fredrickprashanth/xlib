
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/futex.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <string.h>

#define _GNU_SOURCE
#include <sys/syscall.h>
#define futex(uaddr, op, val, timeout, uaddr2, val3)  \
		syscall(SYS_futex , uaddr, op, val, NULL, NULL, NULL)

#include "xlock.h"
#include "debug.h"

static void inline
print_error(char *name) {
	printf("ERR:%s:%d %s errno %d\n",
		__FUNCTION__,
		__LINE__,
		name,
		errno);
}

#define MAP_SZ (4096)
xlock_t *
xlock_create_open(char *name, int create_flag) {
	int fd;
	xlock_t *x;
	int *lock_ptr;
	int do_init = 0;
	x = malloc(sizeof(xlock_t));
	if (!x) {
		print_error("malloc:");
		free(x);
		return NULL;
	}
	strncpy(x->name, name, sizeof(x->name));
	if (create_flag) {
		if ((fd = open(name, O_RDWR, 0666)) > 0){
			free(x);
			return NULL;
		}
		if ((fd = open(name, O_CREAT|O_RDWR, 0666)) < 0){
			free(x);
			print_error("open:");
			return NULL;
		}
		ftruncate(fd, MAP_SZ);
		close(fd);
	}

	if ((fd = open(name, O_RDWR, 0666)) < 0){
		free(x);
		print_error("open:");
		return NULL;
	}
	lock_ptr = mmap(NULL, MAP_SZ, PROT_READ|PROT_WRITE,
			MAP_SHARED, fd, 0);
	if (lock_ptr == MAP_FAILED) {
		free(x);
		print_error("mmap:");
		return NULL;
	}
	x->fd = fd;
	x->lock_ptr = lock_ptr;
	if (create_flag)
		*(x->lock_ptr) = 0;
	return x;
}
xlock_t*
xlock_create(char *name) {
	return xlock_create_open(name, 1);
}
xlock_t*
xlock_open(char *name) {
	return xlock_create_open(name, 0);
}

int 
xlock_close(xlock_t *x){
	if (*x->lock_ptr) {
		print_error("closing a locked-lock: ");
		return -1;
	}
	close(x->fd);
	free(x);
}

#define IS_LOCKED(x) (*((x)->lock_ptr))
#define LOCK(x) (*((x)->lock_ptr) = 1)
#define UNLOCK(x) (*((x)->lock_ptr) = 0)
#ifdef LOCK_DEBUG

static void inline
print_xlock_debug(xlock_t *x) {
	printf("curr pid %d val %d lock_pid %d unlock_pid %d\n",
		getpid(), *(x->lock_ptr), x->lock_pid, x->unlock_pid);
}
static void inline 
pre_lock_debug (xlock_t *x) {
	printf("%s:pid %d before lock %d\n", __FUNCTION__, getpid(), *(x->lock_ptr));
	if (IS_LOCKED(x)|| (x->lock_pid != x->unlock_pid))
		print_xlock_debug(x);
}
static void inline 
post_lock_debug(xlock_t *x) {
	printf("%s: pid %d lock = %d futex\n",
		 __FUNCTION__, getpid(), *(x->lock_ptr));
	x->lock_pid = getpid();
}
static void inline 
pre_unlock_debug(xlock_t *x) {
	printf("%s: pid %d lock %d\n", __FUNCTION__, getpid(), *(x->lock_ptr));
	if (!IS_LOCKED(x) ||(x->lock_pid != getpid()))  {
		print_error("futex already unlocked:");
		print_xlock_debug(x);
	}
}

static void inline
post_unlock_debug(xlock_t *x) {
	printf("%s: pid %d lock %d\n", __FUNCTION__, getpid(), *(x->lock_ptr));
	x->unlock_pid = getpid();
}
#else
static void inline 
pre_lock_debug (xlock_t *x) {
}
static void inline 
post_lock_debug(xlock_t *x) {
}
static void inline 
pre_unlock_debug(xlock_t *x) {
}
static void inline
post_unlock_debug(xlock_t *x) {
}
static void inline 
print_xlock_debug(xlock_t *x) {
}
#endif 


int
raw_lock_it(xlock_t *x) {
	int ret;
	ret = __sync_bool_compare_and_swap(x->lock_ptr, 0, 1);
	return ret;
}
int
raw_unlock_it(xlock_t *x) {
	int ret;
	ret = __sync_bool_compare_and_swap(x->lock_ptr, 1, 0);
	return ret;
}
	
	
		
int
xlock_lock(xlock_t *x) {
	int ret;

	pre_lock_debug(x);
	do { 
		if (raw_lock_it(x))
			break;
		ret = futex(x->lock_ptr, FUTEX_WAIT, 1, NULL, NULL, 0);
	}while (1);
	post_lock_debug(x);
	return ret;
}

int
xlock_unlock(xlock_t *x) {
	pre_unlock_debug(x);
	do {
		if (raw_unlock_it(x)) {
			futex(x->lock_ptr, FUTEX_WAKE, -1, NULL, NULL, 0);
			break;
		}
		else {
			print_xlock_debug(x);
		}
	}while(1);
	post_unlock_debug(x);
	return 0;
}
	 
void xlock_destroy(xlock_t *x) {
	close(x->fd);
	unlink(x->name);
	free(x);
}
