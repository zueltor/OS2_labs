#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#define NUM_LINES 10
#define PARENT (void*)1
#define CHILD (void*)0

void *printLines(void *thread) {
    char *name = (PARENT == thread) ? "Parent" : "Child";
    for (int i = 1; i <= NUM_LINES; i++) {
        printf("%s thread: line %d\n", name, i);
    }
    return NULL;
}

void printError(char *text, int error) {
    fprintf(stderr, "%s: %s\n", text, strerror(error));
}

int main() {
    pthread_t thread;
    int error = pthread_create(&thread, NULL, printLines, CHILD);
    if (error) {
        printError("Could not create thread", error);
        return EXIT_FAILURE;
    }
    error = pthread_join(thread, NULL);
    if (error) {
        printError("Could not join thread", error);
        return EXIT_FAILURE;
    }
    printLines(PARENT);
    return EXIT_SUCCESS;
}