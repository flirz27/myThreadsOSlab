#define _GNU_SOURCE

#include "myThread.h"

#include <err.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>


static int thread_wrapper(void* args){
    struct Thread_args* thread_args = (struct Thread_args *)args;
    void *(*func)(void *) = thread_args->start_routine;
    void *real_arg = thread_args->arg;

    func(real_arg);
    return 0; 
}

int mythread_create(Mythread_t* thread, void *(*start_routine) (void *), void *arg){

    if(thread == NULL){
        mythread_errno = MYTHREAD_ERROR_INVALID_ARG;
        fprintf(stderr, "mythread_create error: %d\n", mythread_errno);
        return -1;
    }

    if(start_routine == NULL){
        mythread_errno = MYTHREAD_ERROR_INVALID_ARG;
        fprintf(stderr, "mythread_create error: %d\n", mythread_errno);
        return -1;
    }

    void* stack = mmap(NULL,
        STACK_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if(stack == MAP_FAILED){
        mythread_errno = MYTHREAD_ERROR_MMAP;
        fprintf(stderr, "mythread_create error: %d\n", mythread_errno);
        return -1;
    }

    void* stack_top = stack + STACK_SIZE;

    struct Thread_args* thread_args = (struct Thread_args*)((char*)stack_top - 256);
    thread_args->arg = arg;
    thread_args->start_routine = start_routine;

    int flags = CLONE_VM | CLONE_FILES | CLONE_SIGHAND | SIGCHLD;

    pid_t tid = clone(thread_wrapper, stack_top, flags, thread_args);
    if(tid == -1){
        mythread_errno = MYTHREAD_CLONE_ERROR;
        fprintf(stderr, "mythread_create error: %d\n", mythread_errno);
        munmap(stack, STACK_SIZE);
        return -1;
    }

    thread->tid = tid;
    thread->stack = stack;
    
    return 0;
}

int mythread_join(Mythread_t thread, void **retval) {
    
    pid_t result = waitpid(thread.tid, NULL, 0);
    
    if (result == -1) {
        mythread_errno = MYTHREAD_WAITPID_ERROR;
        fprintf(stderr, "mythread_join error: %d\n", mythread_errno);
        return -1;
    }
    if (munmap(thread.stack, STACK_SIZE) == -1) {
        mythread_errno = MYTHREAD_ERROR_MUNMAP;
        fprintf(stderr, "mythread_join error: %d\n", mythread_errno);
        return -1;
    }

    if (retval){
        *retval = NULL; 
    } 
    return 0;
}
