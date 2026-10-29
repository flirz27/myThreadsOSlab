#ifndef MYTHREAD_H
#define MYTHREAD_H
#include <sys/types.h>


#define STACK_SIZE (1024*1024)
typedef enum {
    MYTHREAD_SUCCESS = 0,
    MYTHREAD_ERROR_MMAP = 1,
    MYTHREAD_ERROR_MUNMAP = 2,
    MYTHREAD_CLONE_ERROR = 3,
    MYTHREAD_WAITPID_ERROR = 4,
    MYTHREAD_ERROR_INVALID_ARG = 5,
}mythread_error_t;

mythread_error_t mythread_errno = MYTHREAD_SUCCESS;

mythread_error_t mythread_error(void) {
    return mythread_errno;
}


typedef struct Mythread_t
{
    pid_t tid;
    void *stack;
} Mythread_t;

typedef struct Thread_args
{
    void *arg;
    void *(*start_routine)(void *);
} Thread_args;

int mythread_create(Mythread_t *thread, void *(*start_routine)(void *), void *arg);
int mythread_join(Mythread_t thread, void **retval);

#endif
