#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>

#define LINES_COUNT 10
#define PARENT 0
#define CHILD 1
#define THREADS_COUNT 2
#define FAILURE (void*)-1

typedef struct {
    bool parent;
    pthread_mutex_t *mutex;
    pthread_cond_t *condition;
    bool *current_turn;
} Thread_parameters;

bool parent_turn = true;

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
    bool my_turn = parameters->parent;
    bool *current_turn = parameters->current_turn;
    pthread_mutex_t *mutex = parameters->mutex;
    pthread_cond_t *cond = parameters->condition;
    error = pthread_mutex_lock(mutex);
    if (error) {
        printError("Could not lock mutex", error);
        return FAILURE;
    }
    for (int i = 1; i <= LINES_COUNT; i++) {
        while (my_turn != *current_turn) {
            error = pthread_cond_wait(cond, mutex);
            if (error) {
                printError("Could not block on condition variable", error);
                return FAILURE;
            }
        }
        *current_turn = !(*current_turn);
        printf("%s thread: line %d\n", name, i);
        error = pthread_cond_signal(cond);
        if (error) {
            printError("Failed to unblock other thread", error);
            return FAILURE;
        }
    }

    error = pthread_mutex_unlock(mutex);
    if (error) {
        printError("Could not unlock mutex", error);
        return FAILURE;
    }
    return NULL;
}

int clearResources(pthread_mutex_t *mutex, pthread_cond_t *condition) {
    int error;
    int return_value = EXIT_SUCCESS;
    if (mutex != NULL) {
        error = pthread_mutex_destroy(mutex);
        if (error) {
            return_value = EXIT_FAILURE;
        }
    }

    if (condition != NULL) {
        error = pthread_cond_destroy(condition);
        if (error) {
            return_value = EXIT_FAILURE;
        }
    }
    return return_value;
}

int main() {
    pthread_t thread;
    Thread_parameters parameters[THREADS_COUNT];
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
    int error;

    parameters[PARENT].mutex = &mutex;
    parameters[PARENT].condition = &condition;
    parameters[PARENT].current_turn = &parent_turn;
    parameters[PARENT].parent = true;
    parameters[CHILD].mutex = &mutex;
    parameters[CHILD].condition = &condition;
    parameters[CHILD].current_turn = &parent_turn;
    parameters[CHILD].parent = false;

    error = pthread_create(&thread, NULL, printLines, &parameters[CHILD]);
    if (error) {
        printError("Could not create thread", error);
        clearResources(&mutex, &condition);
        return EXIT_FAILURE;
    }
    void *thread_return_value;
    int return_value = EXIT_SUCCESS;
    thread_return_value = printLines(&parameters[PARENT]);
    if (thread_return_value == FAILURE) {
        return_value = EXIT_FAILURE;
    }

    error = pthread_join(thread, &thread_return_value);
    if (error) {
        printError("Could not join thread", error);
        clearResources(&mutex, &condition);
        return EXIT_FAILURE;
    }
    if (thread_return_value == FAILURE) {
        return_value = EXIT_FAILURE;
    }
    error = clearResources(&mutex, &condition);
    if (error == EXIT_FAILURE) {
        return_value = EXIT_FAILURE;
    }
    return return_value;
}
