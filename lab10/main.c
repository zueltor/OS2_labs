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
#define MUTEXES_COUNT 3
#define PARENT_PRIMARY_MUTEX_INDEX 0
#define CHILD_PRIMARY_MUTEX_INDEX 2
#define THREADS_COUNT 2
#define SLEEP_TIME 100000000

typedef struct {
    pthread_mutex_t *mutex;
    int first_index;
    bool parent;
} Thread_parameters;

void printError(char *text, int error) {
    if (NULL == text) {
        return;
    }
    fprintf(stderr, "%s: %s\n", text, strerror(error));
}

int getNextIndex(int index) {
    return (index + 1) % MUTEXES_COUNT;
}

void *printLines(void *args) {
    if (NULL == args) {
        return NULL;
    }
    Thread_parameters *parameters = (Thread_parameters *) args;
    char *name = (parameters->parent == true) ? "Parent" : "Child";
    pthread_mutex_t *mutex = parameters->mutex;
    int index = parameters->first_index;
    int next_index = getNextIndex(index);
    int error = pthread_mutex_lock(&mutex[index]);
    if (error) {
        printError("Could not lock mutex", error);
        exit(EXIT_FAILURE);
    }
    if (parameters->parent != true) {
        error = pthread_mutex_lock(&mutex[next_index]);
        if (error) {
            printError("Could not lock mutex", error);
            exit(EXIT_FAILURE);
        }
        index = getNextIndex(next_index);
    } else {
        index = next_index;
    }
    next_index = getNextIndex(index);

    for (int i = 1; i <= LINES_COUNT; i++) {
        printf("%s thread: line %d\n", name, i);
        error = pthread_mutex_unlock(&mutex[next_index]);
        if (error) {
            printError("Could not unlock mutex", error);
            exit(EXIT_FAILURE);
        }
        error = pthread_mutex_lock(&mutex[index]);
        if (error) {
            printError("Could not lock mutex", error);
            exit(EXIT_FAILURE);
        }
        index = next_index;
        next_index = getNextIndex(index);
    }
    index = next_index;
    next_index = getNextIndex(index);
    error = pthread_mutex_unlock(&mutex[index]);
    if (error) {
        printError("Could not unlock mutex", error);
        exit(EXIT_FAILURE);
    }
    error = pthread_mutex_unlock(&mutex[next_index]);
    if (error) {
        printError("Could not unlock mutex", error);
        exit(EXIT_FAILURE);
    }
    return NULL;
}

void waitForMutexLock(pthread_mutex_t *mutex) {
    if (mutex == NULL) {
        return;
    }
    int error;
    struct timespec sleep_time = {0, SLEEP_TIME};
    do {
        error = pthread_mutex_trylock(mutex);
        if (error != EBUSY && !error) {
            error = pthread_mutex_unlock(mutex);
            if (error) {
                printError("Could not unlock mutex", error);
                exit(EXIT_FAILURE);
            }
        } else if (error != EBUSY) {
            printError("Could not lock mutex", error);
            exit(EXIT_FAILURE);
        }
        nanosleep(&sleep_time, NULL);
    } while (error != EBUSY);
}

int main() {
    pthread_t thread;
    Thread_parameters parameters[THREADS_COUNT];
    pthread_mutex_t mutex[MUTEXES_COUNT];
    for (int i = 0; i < MUTEXES_COUNT; i++) {
        pthread_mutex_t mutex_intializer = PTHREAD_MUTEX_INITIALIZER;
        mutex[i] = mutex_intializer;
    }

    int error = pthread_mutex_lock(&mutex[PARENT_PRIMARY_MUTEX_INDEX]);
    if (error) {
        printError("Could not lock mutex", error);
        exit(EXIT_FAILURE);
    }

    parameters[PARENT].mutex = mutex;
    parameters[PARENT].first_index = getNextIndex(PARENT_PRIMARY_MUTEX_INDEX);
    parameters[PARENT].parent = true;
    parameters[CHILD].mutex = mutex;
    parameters[CHILD].first_index = CHILD_PRIMARY_MUTEX_INDEX;
    parameters[CHILD].parent = false;

    error = pthread_create(&thread, NULL, printLines, &parameters[CHILD]);
    if (error) {
        printError("Could not create thread", error);
        return EXIT_FAILURE;
    }

    waitForMutexLock(&mutex[CHILD_PRIMARY_MUTEX_INDEX]);

    printLines(&parameters[PARENT]);

    error = pthread_join(thread, NULL);
    if (error) {
        printError("Could not join thread", error);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
