#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define NOT_INTERRUPTED 1
#define NUM_LINES_THRESHOLD 1000
#define TRUE 1
#define FALSE 0

void printError(char *text, int error);

void printEnded(void *args) {
    int many_lines_printed = *(int *) args;
    printf("Child thread finished printing.\n");
    if (many_lines_printed) {
        printf("Number of lines printed: at least %d\n", NUM_LINES_THRESHOLD);
    }
    free(args);
}

void *foreverPrintLines(void *args) {
    int *printed_many_lines;
    int error = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    if (error) {
        printError("Could not set cancel state", error);
        return NULL;
    }
    printed_many_lines = (int *) malloc(sizeof(int));
    *printed_many_lines = FALSE;
    pthread_cleanup_push(printEnded, printed_many_lines);
        error = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        if (error) {
            printError("Could not set cancel state", error);
            pthread_exit(NULL);
        }
        int line_number = 1;
        while (NOT_INTERRUPTED) {
            printf("Line #%d\n", line_number++);
            if (line_number > NUM_LINES_THRESHOLD) {
                *printed_many_lines = TRUE;
            }
            pthread_testcancel();
        }
    pthread_cleanup_pop(TRUE);
}

void printError(char *text, int error) {
    fprintf(stderr, "%s: %s\n", text, strerror(error));
}

int main() {
    pthread_t thread;
    int error = pthread_create(&thread, NULL, foreverPrintLines, NULL);
    if (error) {
        printError("Could not create thread", error);
        return EXIT_FAILURE;
    }
    sleep(2);
    pthread_cancel(thread);
    error = pthread_join(thread, NULL);
    if (error) {
        printError("Could not join thread", error);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}