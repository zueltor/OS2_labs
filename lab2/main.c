#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#define NUM_LINES 10

void *parentPrintLines(void *args) {
    int num_lines = NUM_LINES;
    for (int i = 1; i <= num_lines; i++) {
        printf("Parent thread: line %d\n", i);
    }
    return NULL;
}

void *childPrintLines(void *args) {
    int num_lines = NUM_LINES;
    for (int i = 1; i <= num_lines; i++) {
        printf("Child thread: line %d\n", i);
    }
    return NULL;
}

void printError(char *text, int error) {
    fprintf(stderr, "%s: %s\n", text, strerror(error));
}

int main() {
    pthread_t tid;
    int error = pthread_create(&tid, NULL, childPrintLines, NULL);
    if (error) {
        printError("Could not create thread", error);
        return EXIT_FAILURE;
    }
    error = pthread_join(tid, NULL);
    if (error) {
        printError("Could not join thread", error);
        return EXIT_FAILURE;
    }
    parentPrintLines(NULL);
    pthread_exit(NULL);
}