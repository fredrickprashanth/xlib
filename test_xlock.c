#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "xlock.h"
#include "debug.h"

void
print_inc_file() {
	
	int fd, fintt;
	int *fint = &fintt;
	struct stat stbuf;
	if (stat("test_inc",  &stbuf)) 
		return;
	fd = open("test_inc", O_RDONLY, 0666);
	if (fd < 0){
		perror("open");
		return;
	}
	read(fd, fint, sizeof(int));
	close(fd);
	printf("fint = %d\n", *fint);
}
	
void
do_inc_file(void) {
	int fd, fintt;
	int *fint = &fintt;
	struct stat stbuf;
	int do_init = 0;
	if (stat("test_inc",  &stbuf)) 
		do_init = 1;
	fd = open("test_inc", O_RDWR|O_CREAT, 0666);
	if (fd < 0){
		perror("open");
		return;
	}
	read(fd, fint, sizeof(int));
	if (do_init) *fint = 0;
	print_debug("pid %d : before file int = %d\n", getpid(), *fint);
	*fint = *fint + 1;
	print_debug("pid %d : after file int = %d\n", getpid(), *fint);
	lseek(fd, 0, SEEK_SET);
	write(fd, fint, sizeof(int));
	close(fd);
	fflush(stdout);
}
#define MAX_ITER 20
#define ITER_SLEEP 10000
int iter_sleep;

void test1(void) {
	int i;
	for (i = 0; i < MAX_ITER; i++) {
		do_inc_file();
		usleep(iter_sleep);
	}
}

void test2(void) {
	int i;
	xlock_t *mylock;
	mylock = xlock_open("this_lock");
	print_debug("pid %d starting\n", getpid());
	for (i = 0; i < MAX_ITER; i++) {
		print_debug("pid %d iter = %d\n", getpid(), i);
		xlock_lock(mylock);
		print_debug("pid %d got lock \n", getpid());
		do_inc_file();
		print_debug("pid %d leaving lock \n", getpid());
		xlock_unlock(mylock);
		print_debug("pid %d left lock \n", getpid());
		usleep(iter_sleep);
		print_debug("pid %d sleep done\n", getpid());
		fflush(stdout);
	}
}
	
#define NARGS (1+2)
int 
main(int argc, char *argv[]) {
	int nr_forks;
	int i, ret;
	if (argc<NARGS) {
		printf("test_xlock <nr_forks> <sleep_usec>\n");
		return -1;
	}
	sscanf(argv[1], "%d", &nr_forks);
	sscanf(argv[2], "%d", &iter_sleep);
	
	for (i=0; i<nr_forks; i++) {
		if (!(ret = fork())) {
			test2();
			return 0;
		}
	}
	for(i=0;i<nr_forks; i++)
		wait();
	print_inc_file();
	return 0;
}

