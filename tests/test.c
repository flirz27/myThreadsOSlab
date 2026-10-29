#define _GNU_SOURCE

#include "myThread.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

#define NUM_THREADS 5
#define TEST_FILE "test_output.txt"

int shared_counter = 0;
int shared_array[NUM_THREADS] = {0};
int global_fd = -1;

void* noop(void* arg) {
    sleep(1);
    return NULL;
 }

bool test_null_thread_ptr(void) {
    mythread_create(NULL, NULL, NULL);
    return mythread_error() == MYTHREAD_ERROR_INVALID_ARG;
}

bool test_null_routine_ptr(void){
    Mythread_t thread;
    mythread_create(&thread, NULL, NULL);
    return mythread_error() == MYTHREAD_ERROR_INVALID_ARG;
}

bool test_wrong_tid(void){
    Mythread_t thread;
    thread.tid = 0x7fffffff;
    thread.stack = NULL;
    mythread_join(thread, NULL);
    return mythread_error() == MYTHREAD_WAITPID_ERROR;
}

bool test_bad_stack(void){
    Mythread_t thread;
    mythread_create(&thread, noop, NULL);

    void* real_stack = thread.stack;
    thread.stack = thread.stack + 1;
    mythread_join(thread, NULL);

    munmap(real_stack, STACK_SIZE);
    return mythread_error() == MYTHREAD_ERROR_MUNMAP;
}

void* test_basic_func(void* arg) {
    int id = *(int*)arg;
    printf("[Test 1] flowers blooms in your hearth! thread %d! TID: %d\n", id, gettid());
    return NULL;
}

void* test_shared_mem_func(void* arg) {
    int id = *(int*)arg;
    
    shared_array[id] = id * 10; 
    
    for (int i = 0; i < 1000; i++) {
        shared_counter++;
    }
    
    return NULL;
}

void* test_shared_files_func(void* arg) {
    int id = *(int*)arg;
    char buffer[64];
    int len = snprintf(buffer, sizeof(buffer), "Written by thread %d (TID: %d)\n", id, gettid());
    
    if (global_fd != -1) {
        write(global_fd, buffer, len);
    }
    return NULL;
}

void* test_stress_func(void* arg) {
    usleep(100000); 
    return NULL;
}

void print_test_header(const char* test_name) {
    printf("\n========================================\n");
    printf("Running: %s\n", test_name);
    printf("========================================\n");
}

int main() {
    Mythread_t threads[NUM_THREADS];
    int args[NUM_THREADS];
    int status = 0;

    print_test_header("Test 1: Basic Create & Join");
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i] = i + 1;
        if (mythread_create(&threads[i], test_basic_func, &args[i]) != 0) {
            fprintf(stderr, "Test 1 FAILED: Could not create thread %d\n", i);
            return 1;
        }
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        mythread_join(threads[i], NULL);
    }
    printf("Test 1 PASSED: All threads created and joined.\n");

    print_test_header("Test 2: Shared Memory");
    shared_counter = 0;
    memset(shared_array, 0, sizeof(shared_array));

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i] = i;
        mythread_create(&threads[i], test_shared_mem_func, &args[i]);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        mythread_join(threads[i], NULL);
    }

    int mem_test_passed = 1;
    for (int i = 0; i < NUM_THREADS; i++) {
        if (shared_array[i] != i * 10) {
            mem_test_passed = 0;
            break;
        }
    }
    if (mem_test_passed && shared_counter > 0) {
        printf("Test 2 PASSED: shared_array updated correctly. shared_counter = %d (memory is shared!)\n", shared_counter);
    } else {
        printf("Test 2 FAILED: Memory is NOT shared properly.\n");
        status = 1;
    }

    print_test_header("Test 3: Shared File Descriptors (CLONE_FILES)");
    global_fd = open(TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (global_fd == -1) {
        perror("open failed");
        return 1;
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i] = i + 1;
        mythread_create(&threads[i], test_shared_files_func, &args[i]);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        mythread_join(threads[i], NULL);
    }
    close(global_fd);
    FILE *f = fopen(TEST_FILE, "r");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fclose(f);
    
    if (fsize > 0) {
        printf("Test 3 PASSED: Threads wrote to the file using parent's fd. File size: %ld bytes.\n", fsize);
        printf("Content of %s:\n", TEST_FILE);
        system("cat " TEST_FILE); 
    } else {
        printf("Test 3 FAILED: File is empty.\n");
        status = 1;
    }


    print_test_header("Test 4: Error Handling (Resource Exhaustion)");
    int created_count = 0;
    Mythread_t* stress_threads = malloc(1000 * sizeof(Mythread_t));
    if (!stress_threads) {
        printf("Test 4 SKIPPED: Could not allocate memory for stress test array.\n");
    } else {
        while (created_count < 1000) {
            if (mythread_create(&stress_threads[created_count], test_stress_func, NULL) != 0) {
                printf("Test 4 PASSED: mythread_create correctly returned error after %d threads (errno: %s).\n", 
                       created_count, strerror(errno));
                break;
            }
            created_count++;
        }
        
        if (created_count == 1000) {
            printf("Test 4: Created 1000 threads.\n");
        }
        for (int i = 0; i < created_count; i++) {
            mythread_join(stress_threads[i], NULL);
        }
        free(stress_threads);
    }

    print_test_header("Test 5: NULL thread_ptr");
    if(test_null_thread_ptr()){
        printf("Test 5: passed test with error %d\n", mythread_error());
    }else{
        status = 1;
        printf("Test 5: not passed\n");
    }

    print_test_header("Test 6: NULL routine_ptr");
    if(test_null_routine_ptr()){
        printf("Test 6: passed test with error %d\n", mythread_error());
    }else{
        status = 1;
        printf("Test 6: not passed\n");
    }

    print_test_header("Test 7: wrong tid");
    if(test_wrong_tid()){
        printf("Test 7: passed test with error %d\n", mythread_error());
    }else{
        status = 1;
        printf("Test 7: not passed\n");
    }

    print_test_header("Test 8: bad munmap");
    if(test_bad_stack()){
        printf("Test 8: passed test with error %d\n", mythread_error());
    }else{
        status = 1;
        printf("Test 8: not passed\n");
    }

    printf("\n========================================\n");
    if (status == 0) {
        printf("ALL TESTS PASSED SUCCESSFULLY!\n");
    } else {
        printf("SOME TESTS FAILED!\n");
    }
    printf("========================================\n");

    return status;
}
