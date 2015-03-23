#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "psem.h"
#include "debug.h"

#define _GNU_SOURCE
#include <sys/syscall.h>
#define gettid()  \
		syscall(SYS_gettid ,NULL, NULL, NULL, NULL, NULL)

void
psem_init(psem_t *x, int count) {
	x->count = count;
}
psem_t *
psem_create(char *name, int count) {
	psem_t *x;
	x = (psem_t *)malloc(sizeof(psem_t));
	if (!x) {
		return x;
	}
	x->count = count;
	pthread_cond_init(&x->sem_cond, NULL);
	pthread_mutex_init(&x->sem_mutex, NULL);
	return x;
}

void
psem_up(psem_t *x) {
	pthread_mutex_lock(&x->sem_mutex);
	x->count++;
	print_debug("thread %d sem_count %d\n", gettid(), x->count);
	pthread_mutex_unlock(&x->sem_mutex);
	pthread_cond_broadcast(&x->sem_cond);
}

void
psem_down(psem_t *x) {
	pthread_mutex_lock(&x->sem_mutex);
	while (1) {
		if (x->count > 0) {
			x->count--;
			break;
		}
		else {
			pthread_cond_wait(&x->sem_cond, &x->sem_mutex);
		}
	}
	print_debug("thread %d sem_count %d\n", gettid(), x->count);
	pthread_mutex_unlock(&x->sem_mutex);
}


void
psem_destroy(psem_t *ps) {
	free(ps);
}
	
