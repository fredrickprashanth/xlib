#include "xlock.h"

#ifndef __XSEM_H__
#define __XSEM_H__

struct xsem {
	char name[32];
	int *count;
	int sem_fd;
	xlock_t *sem_lock;
};
typedef struct xsem xsem_t;


xsem_t *
xsem_create(char *name, int count);

xsem_t *
xsem_open(char *name, int count);

void
xsem_up(xsem_t *x);

void
xsem_down(xsem_t *x);


void
xsem_destroy(xsem_t *x);

#endif
