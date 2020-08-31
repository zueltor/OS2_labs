#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define NOT_INTERRUPTED 1

void *foreverPrintLines(void *args) {
    int line_number = 1;
    while (NOT_INTERRUPTED) {
        printf("Line #%d\n", line_number++);
        pthread_testcancel();
    }
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