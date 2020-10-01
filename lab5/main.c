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
    bool many_lines_printed = *(bool *) args;
    printf("Child thread finished printing.\n");
    if (many_lines_printed) {
        printf("Number of lines printed: at least %d\n", LINES_COUNT_THRESHOLD);
    }
    free(args);
}

void *foreverPrintLines(void *args) {
    bool printed_many_lines = false;
    bool NOT_CANCELLED = true;
    pthread_cleanup_push(printEnded, &printed_many_lines);
        int line_number = 1;
        while (NOT_CANCELLED) {
            for (int i = 0; i < LINES_COUNT_THRESHOLD; i++) {
                printf("Line #%d\n", line_number++);
                pthread_testcancel();
            }
            printed_many_lines = true;
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

    error = pthread_cancel(thread);
    if (error) {
        printError("Could not cancel thread", error);
        return_value = EXIT_FAILURE;
    }
    error = pthread_join(thread, NULL);
    if (error) {
        printError("Could not join thread", error);
        return_value = EXIT_FAILURE;
    }
    return return_value;
}
