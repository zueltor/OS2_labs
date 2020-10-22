#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define LINES_COUNT 10
#define PARENT 1
#define CHILD 0

typedef struct {
    char *thread_name;
    bool can_print;
    bool cannot_print;
    pthread_mutex_t *permission_lock;
} Thread_parameters;

void printError(char *text, int error) {
    if (NULL == text) {
        return;
    }
    fprintf(stderr, "%s: %s\n", text, strerror(error));
}

void *parentPrintLines(void *args) {
    if (NULL == args) {
        return NULL;
    }
    Thread_parameters *parameters = (Thread_parameters *) args;
    char *name = parameters->thread_name;
    bool can_print = parameters->can_print;
    bool cannot_print = parameters->cannot_print;
    pthread_mutex_t *permission_lock = parameters->permission_lock;
    static bool permission = true;
    int error;

    int i = 1;
    while (i <= LINES_COUNT) {
        error = pthread_mutex_lock(permission_lock);
        if (error) {
            printError("Could not lock mutex", error);
            exit(EXIT_FAILURE);
        }
        if (permission == can_print) {
            printf("%s thread: line %d\n", name, i);
            i++;
            permission = cannot_print;
        }
        error = pthread_mutex_unlock(permission_lock);
        if (error) {
            printError("Could not unlock mutex", error);
            exit(EXIT_FAILURE);
        }
    }
    return NULL;
}

int main() {
    pthread_t thread;
    Thread_parameters parameters[2];
    pthread_mutex_t permission_lock = PTHREAD_MUTEX_INITIALIZER;
    parameters[CHILD].can_print = false;
    parameters[CHILD].cannot_print = true;
    parameters[CHILD].thread_name = "Child";
    parameters[CHILD].permission_lock = &permission_lock;
    parameters[PARENT].can_print = true;
    parameters[PARENT].cannot_print = false;
    parameters[PARENT].thread_name = "Parent";
    parameters[PARENT].permission_lock = &permission_lock;

    int error = pthread_create(&thread, NULL, parentPrintLines, &parameters[CHILD]);
    if (error) {
        printError("Could not create thread", error);
        return EXIT_FAILURE;
    }
    parentPrintLines(&parameters[PARENT]);
    error = pthread_join(thread, NULL);
    if (error) {
        printError("Could not join thread", error);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
