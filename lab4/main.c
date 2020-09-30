#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define SLEEP_TIME 2

void printError(char *text, int error) {
    if (NULL == text) {
        return;
    }
    fprintf(stderr, "%s: %s\n", text, strerror(error));
}

void *foreverPrintLines(void *args) {
    int line_number = 1;
    int NOT_CANCELLED = 1;
    while (NOT_CANCELLED) {
        printf("Line #%d\n", line_number++);
        pthread_testcancel();
    }
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
