#ifndef __PSEM_H__
#define __PSEM_H__

typedef struct psem {
	int count;
	pthread_mutex_t sem_mutex;
	pthread_cond_t sem_cond;
} psem_t;


psem_t *
psem_create(char *name, int count);

psem_t *
psem_open(char *name, int count);

void
psem_up(psem_t *x);

void
psem_down(psem_t *x);


void
psem_destroy(psem_t *ps);

#endif
