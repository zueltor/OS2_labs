#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <semaphore.h>

#define LINES_COUNT 10
#define PARENT 0
#define CHILD 1
#define THREADS_COUNT 2
#define FAILURE (void*)-1
#define SHARED_SEMAPHORE 0
#define UNLOCKED_SEMAPHORE 1
#define LOCKED_SEMAPHORE 0

typedef struct {
    bool parent;
    sem_t *sem1;
    sem_t *sem2;
    pthread_t *other_thread;
} Thread_parameters;

void printError(char *text, int error) {
    if (NULL == text) {
        return;
    }
    fprintf(stderr, "%s: %s\n", text, strerror(error));
}

void *printLines(void *args) {
    if (NULL == args) {
        fprintf(stderr, "Expected not null argument\n");
        return FAILURE;
    }
    int error;
    Thread_parameters *parameters = (Thread_parameters *) args;
    char *name = (parameters->parent == true) ? "Parent" : "Child";
    sem_t *sem1 = parameters->sem1;
    sem_t *sem2 = parameters->sem2;

    for (int i = 1; i <= LINES_COUNT; i++) {
        error = sem_wait(sem1);
        if (error) {
            printError("Could not lock semaphore", error);
            pthread_cancel(*(parameters->other_thread));
            return FAILURE;
        }
        printf("%s thread: line %d\n", name, i);
        error = sem_post(sem2);
        if (error) {
            printError("Could not unlock semaphore", error);
            pthread_cancel(*(parameters->other_thread));
            return FAILURE;
        }
    }

    return NULL;
}

void clearResources(void *args) {
    int error;
    Thread_parameters *parameters = (Thread_parameters *) args;
    if (parameters == NULL) {
        return;
    }
    error = pthread_join(*(parameters->other_thread), NULL);
    if (error) {
        printError("Could not join thread", error);
    }
    error = sem_destroy(parameters->sem1);
    if (error) {
        printError("Could not destroy semaphore", error);
    }
    error = sem_destroy(parameters->sem2);
    if (error) {
        printError("Could not destroy semaphore", error);
    }
}

void *parentPrintLines(void *args) {
    Thread_parameters *parameters = (Thread_parameters *) args;
    void *return_value;
    pthread_cleanup_push(clearResources, args);
        return_value = printLines(parameters);
    pthread_cleanup_pop(true);
    return return_value;
}

int main() {
    pthread_t child_thread;
    pthread_t main_thread = pthread_self();
    Thread_parameters parameters[THREADS_COUNT];
    sem_t sem1, sem2;
    int error;
    error = sem_init(&sem1, SHARED_SEMAPHORE, UNLOCKED_SEMAPHORE);
    if (error) {
        printError("Could not init semaphore", error);
        return EXIT_FAILURE;
    }
    error = sem_init(&sem2, SHARED_SEMAPHORE, LOCKED_SEMAPHORE);
    if (error) {
        printError("Could not init semaphore", error);
        return EXIT_FAILURE;
    }

    parameters[PARENT].sem1 = &sem1;
    parameters[PARENT].sem2 = &sem2;
    parameters[PARENT].parent = true;
    parameters[PARENT].other_thread = &child_thread;
    parameters[CHILD].sem1 = &sem2;
    parameters[CHILD].sem2 = &sem1;
    parameters[CHILD].parent = false;
    parameters[CHILD].other_thread = &main_thread;

    error = pthread_create(&child_thread, NULL, printLines, &parameters[CHILD]);
    if (error) {
        printError("Could not create child_thread", error);
        clearResources(&parameters[PARENT]);
        return EXIT_FAILURE;
    }
    void *thread_return_value;
    int return_value = EXIT_SUCCESS;
    thread_return_value = parentPrintLines(&parameters[PARENT]);
    if (thread_return_value == FAILURE) {
        return_value = EXIT_FAILURE;
    }
    return return_value;
}
