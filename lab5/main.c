#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define LINES_COUNT_THRESHOLD 1000
#define SLEEP_TIME 2
#define TRUE 1
#define FALSE 0
#define printError(text, error) fprintf(stderr, text": %s\n",strerror(error));

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
    int NOT_CANCELLED = 1;
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
        while (NOT_CANCELLED) {
            printf("Line #%d\n", line_number++);
            if (line_number > LINES_COUNT_THRESHOLD) {
                *printed_many_lines = TRUE;
            }
            pthread_testcancel();
        }
    pthread_cleanup_pop(TRUE);
    return NULL;
}

int main() {
    pthread_t thread;
    int error = pthread_create(&thread, NULL, foreverPrintLines, NULL);
    if (error) {
        printError("Could not create thread", error);
        return EXIT_FAILURE;
    }

    int return_value = EXIT_SUCCESS;
    unsigned int unslept_amount = sleep(SLEEP_TIME);
    if (unslept_amount > 0) {
        fprintf(stderr, "Caught signal terminated sleep\n");
        return_value = EXIT_FAILURE;
    }
    pthread_cancel(thread);
    error = pthread_join(thread, NULL);
    if (error) {
        printError("Could not join thread", error);
        return_value = EXIT_FAILURE;
    }
    return return_value;
}
