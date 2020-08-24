#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

//#define pprintf(format, args...) printf("Thread #%ld: ",pthread_self()); printf(format,##args);
#define NUM_LINES 10
typedef struct {
    int thread_identifier;
    int num_lines;
} ThreadArgs_t;

void *printLines(void *args) {
    int num_lines = ((ThreadArgs_t *) args)->num_lines;
    int thread_identifier = ((ThreadArgs_t *) args)->thread_identifier;

    for (int i = 1; i <= num_lines; i++) {
        printf("Thread #%d: line %d\n", thread_identifier, i);
    }
    return NULL;
}

void printError(char *text, int error) {
    fprintf(stderr, "%s: %s\n", text, strerror(error));
}

int main() {
    pthread_attr_t attr;
    pthread_t tid;
    int error = pthread_attr_init(&attr);
    if (error) {
        printError("Could not initialise attributes", error);
        return EXIT_FAILURE;
    }
    ThreadArgs_t args1 = {1, NUM_LINES};
    ThreadArgs_t args2 = {2, NUM_LINES};

    error = pthread_create(&tid, &attr, printLines, &args1);
    if (error) {
        printError("Could not create thread", error);
        return EXIT_FAILURE;
    }
    pthread_attr_destroy(&attr);
    printLines(&args2);
    pthread_exit(NULL);
}
