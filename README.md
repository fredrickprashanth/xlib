# xlib
## OS wide Lock and Semphore using futex
The mutex is implemented on a filesystem namespace.
So the mutex can be shared across processes.
The same applies to the semaphore, ie., the semaphore
can be a OS wide semaphore.
