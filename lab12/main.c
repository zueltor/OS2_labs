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
    pthread_t *other_thread;
    pthread_mutex_t *mutex;
    pthread_cond_t *condition;
    bool need_to_unlock;
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
    parameters->need_to_unlock = true;
    error = pthread_mutex_lock(mutex);
    if (error) {
        printError("Could not lock mutex", error);
        pthread_cancel(*(parameters->other_thread));
        return FAILURE;
    }
    for (int i = 1; i <= LINES_COUNT; i++) {
        while (my_turn != *current_turn) {
            error = pthread_cond_wait(cond, mutex);
            if (error) {
                printError("Could not block on condition variable", error);
                pthread_cancel(*(parameters->other_thread));
                return FAILURE;
            }
        }
        *current_turn = !my_turn;
        printf("%s thread: line %d\n", name, i);
        error = pthread_cond_signal(cond);
        if (error) {
            printError("Failed to unblock other thread", error);
            pthread_cancel(*(parameters->other_thread));
            return FAILURE;
        }
    }

    error = pthread_mutex_unlock(mutex);
    if (error) {
        printError("Could not unlock mutex", error);
        exit(EXIT_FAILURE);
    }
    parameters->need_to_unlock = false;
    return NULL;
}

void clearResources(void *args) {
    int error;
    Thread_parameters *parameters = (Thread_parameters *) args;
    if (parameters == NULL) {
        return;
    }
    if (parameters->need_to_unlock) {
        error = pthread_mutex_unlock(parameters->mutex);
        if (error) {
            printError("Could not unlock mutex", error);
            exit(EXIT_FAILURE);
        }
    }
    error = pthread_join(*(parameters->other_thread), NULL);
    if (error) {
        printError("Could not join thread", error);
    }
    error = pthread_mutex_destroy(parameters->mutex);
    if (error) {
        printError("Could not destroy mutex", error);
    }
    error = pthread_cond_destroy(parameters->condition);
    if (error) {
        printError("Could not destroy condition variable", error);
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
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
    int error;

    parameters[PARENT].mutex = &mutex;
    parameters[PARENT].condition = &condition;
    parameters[PARENT].current_turn = &parent_turn;
    parameters[PARENT].parent = true;
    parameters[PARENT].other_thread = &child_thread;
    parameters[CHILD].mutex = &mutex;
    parameters[CHILD].condition = &condition;
    parameters[CHILD].current_turn = &parent_turn;
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
