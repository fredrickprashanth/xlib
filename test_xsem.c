#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <unistd.h>

#include "psem.h" 
#include "xsem.h"
#include "debug.h"

#define SEM_T SEM_op(SEMTYPE, sem_t)
#define SEM_CREATE SEM_op(SEMTYPE,sem_create)
#define SEM_UP SEM_op(SEMTYPE, sem_up)
#define SEM_DOWN SEM_op(SEMTYPE, sem_down)
#define SEM_DESTROY SEM_op(, sem_destroy)

#if USE_PTHREADS
#define thread_id gettid
#define SEM_op(type,op) p ## op
#else
#define thread_id getpid
#define SEM_op(type,op) x ## op
#endif

#define NARGS 3
char *test_alloc;
#define TEST_ALLOC_NR_PAGES (128)
#define PAGE_SZ (4096)
#define TEST_ALLOC_SZ (TEST_ALLOC_NR_PAGES*PAGE_SZ)
int *check_count;
int max_loops;
void*
sem_test_thread(void *data) {
	SEM_T *ps = data;
	int loops = max_loops;
	print_debug("thread %d enter\n", thread_id());
	SEM_DOWN(ps);
	print_debug("thread %d got sem\n", thread_id());
	memset(test_alloc, 0xaa, TEST_ALLOC_SZ);
	while(loops-->0) {
		usleep(1);
	}
	check_count[0]++;
	print_debug("thread %d check_count = %d\n", thread_id(), check_count[0]);
	SEM_UP(ps);
	print_debug("thread %d leave sem\n", thread_id());
	print_debug("thread %d exit\n", thread_id());
#if USE_PTHREADS
	pthread_exit(NULL);
#else
	return NULL;
#endif
}

int
main(int argc, char *argv[]) {
	
	int nr_threads, arg_pos = 0 ;
	int sem_count;
	int i;
	pthread_t *ths;
	SEM_T *ps;
	
	if (argc < (NARGS+1)) {
		printf("test_xsem <nr_threads> <sem_count> <max_loops>\n");
		return 0;
	}
	sscanf(argv[++arg_pos], "%d", &nr_threads);
	sscanf(argv[++arg_pos], "%d", &sem_count);
	sscanf(argv[++arg_pos], "%d", &max_loops);
	
	ps = SEM_CREATE("test_sem", sem_count);
	if (!ps) {
		printf("xsem create failed\n");
		return -1;
	}
	ths = malloc(sizeof(pthread_t)*nr_threads);
	if (!ths) {
		printf("malloc failure for pthread array alloc\n");
		SEM_DESTROY(ps);
		return -2;
	}
	check_count = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE,
				MAP_ANON|MAP_SHARED, -1, 0);
	if (check_count == MAP_FAILED) {
		printf("check_cout map failed\n");
		return 0;
	}
	check_count[0] = 0;

	test_alloc = malloc(TEST_ALLOC_SZ);
	if (!test_alloc) {
		printf("test_alloc failed\n");
		return 0;
	}
	memset(test_alloc, 0xbb, TEST_ALLOC_SZ);
	for (i = 0; i<nr_threads; i++) {
#if USE_PTHREADS
		ret = pthread_create(ths+i, NULL, sem_test_thread, ps);
		if (ret) {
			printf("pthread_create failed for i = %d\n", i);
			perror("pthread_create");
		}
#else
		if (fork() == 0) {
			sem_test_thread(ps);
			return 0;
		}
#endif
	}
	for (i = 0; i<nr_threads; i++) {
#if USE_PTHREADS
		pthread_join(ths[i], NULL);
#else	
		wait(NULL);
#endif
	}
	SEM_DESTROY(ps);
	printf("check_count = %d\n", check_count[0]);

	return 0;
}
