#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define LINES_COUNT_THRESHOLD 1000
#define SLEEP_TIME 2

void printError(char *text, int error) {
    if (NULL == text) {
        return;
    }
    fprintf(stderr, "%s: %s\n", text, strerror(error));
}

void printEnded(void *args) {
    if (NULL == args) {
        return;
    }
    int many_lines_printed = *(int *) args;
    printf("Child thread finished printing.\n");
    if (many_lines_printed) {
        printf("Number of lines printed: at least %d\n", LINES_COUNT_THRESHOLD);
    }
    free(args);
}

void *foreverPrintLines(void *args) {
    int *printed_many_lines;
    bool NOT_CANCELLED = true;
    int error = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    if (error) {
        printError("Could not set cancel state", error);
        return NULL;
    }
    printed_many_lines = (int *) malloc(sizeof(int));
    if (NULL == printed_many_lines) {
        fprintf(stderr, "Could not allocate memory\n");
        return NULL;
    }
    *printed_many_lines = false;
    pthread_cleanup_push(printEnded, printed_many_lines);
        error = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        if (error) {
            printError("Could not set cancel state", error);
            pthread_exit(NULL);
        }
        int line_number = 1;
        while (NOT_CANCELLED) {
            printf("Line #%d\n", line_number++);
            if (line_number > LINES_COUNT_THRESHOLD) {
                *printed_many_lines = true;
            }
            pthread_testcancel();
        }
    pthread_cleanup_pop(true);
    return NULL;
}

int main() {
    pthread_t thread;
    int return_value = EXIT_SUCCESS;
    int error = pthread_create(&thread, NULL, foreverPrintLines, NULL);
    if (error) {
        printError("Could not create thread", error);
        return EXIT_FAILURE;
    }

    unsigned int unslept_time = SLEEP_TIME;
    do {
        unslept_time = sleep(unslept_time);
    } while (unslept_time > 0);

    pthread_cancel(thread);
    error = pthread_join(thread, NULL);
    if (error) {
        printError("Could not join thread", error);
        return_value = EXIT_FAILURE;
    }
    return return_value;
}
