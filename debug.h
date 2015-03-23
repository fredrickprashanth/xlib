#ifndef __MYCOMMON_H__
#define __MYCOMMON_H__ 

#define _GNU_SOURCE
#include <sys/syscall.h>
#define gettid()  \
		syscall(SYS_gettid ,NULL, NULL, NULL, NULL, NULL)

#ifndef NDEBUG
#define print_debug(a,...) fprintf(stderr, a, ##__VA_ARGS__)
#else
#define print_debug(a,...) do { } while(0)
#endif
#endif
