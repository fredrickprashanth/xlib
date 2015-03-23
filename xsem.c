#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/futex.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <string.h>

#include "xlock.h"
#include "xsem.h"
#include "debug.h"

#define _GNU_SOURCE
#include <sys/syscall.h>
#define futex(uaddr, op, val, timeout, uaddr2, val3)  \
		syscall(SYS_futex , uaddr, op, val, NULL, NULL, NULL)


xsem_t*
xsem_create_open(char *sem_name, int count, int create_flag) {
	xsem_t *x;
	char name[64];
	int fd;

	x = malloc(sizeof(xsem_t));
	if (!x)
		return x;
	strcpy(name, sem_name);
	if (create_flag) { 
		if ((fd = open(name, O_RDWR, 0666)) > 0) {
			free(x);
			return NULL;
		}
		if ((fd = open(name, O_RDWR|O_CREAT, 0666)) < 0) {
			free(x);
			return NULL;
		}
		ftruncate(fd, sizeof(int));
		close(fd);
	}
	if ((fd = open(name, O_RDWR, 0666)) < 0) {
		free(x);
		return NULL;
	}
	x->count = mmap(NULL, sizeof(int),
			PROT_READ|PROT_WRITE,
			MAP_SHARED, fd, 0);
	if (x->count == MAP_FAILED) {
		free(x);
		close(fd);
		return NULL;
	}
	sprintf(name, "%s.lock", sem_name);
	if (create_flag) { 
		x->sem_lock = xlock_create(name);
	}
	else {
		x->sem_lock = xlock_open(name);
	}
	if (!x->sem_lock){
		free(x);
		close(fd);
		return NULL;
	}
	if (create_flag)
		x->count[0] = count;
	x->sem_fd = fd;
	strcpy(x->name, sem_name);
}

xsem_t*
xsem_create(char *name, int count) {
	return xsem_create_open(name, count, 1);
}
xsem_t*
xsem_open(char *name, int count) {
	return xsem_create_open(name, count, 0);
}
void
xsem_down(xsem_t *x) {
	
	xlock_lock(x->sem_lock);
	while (1) {
		if (x->count[0] > 0) {
			x->count[0]--;
			break;
		} else {
			xlock_unlock(x->sem_lock);
			futex(x->count, FUTEX_WAIT, 0, NULL, NULL, 0);
			xlock_lock(x->sem_lock);
		}
	}
	print_debug("thread %d sem_count %d\n", getpid(), x->count[0]);
	xlock_unlock(x->sem_lock);
}

void
xsem_up(xsem_t *x) {
	xlock_lock(x->sem_lock);
	x->count[0]++;
	print_debug("thread %d sem_count %d\n", getpid(), x->count[0]);
	xlock_unlock(x->sem_lock);
	futex(x->count, FUTEX_WAKE, -1, NULL, NULL, 0);
}

void 
xsem_destroy(xsem_t *x) {
	char name[64];
	close(x->sem_fd);
	unlink(x->name);
	xlock_destroy(x->sem_lock);
	free(x);
}
