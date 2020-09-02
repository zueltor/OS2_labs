#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#define NUM_THREADS 4
typedef struct {
    int count;
    char **lines;
} ThreadArgs_t;

void *printLines(void *args) {
    int num_lines = ((ThreadArgs_t *) args)->count;
    char **lines = ((ThreadArgs_t *) args)->lines;
    for (int i = 0; i < num_lines; i++) {
        printf("%s\n", lines[i]);
    }
    return NULL;
}

void printError(char *text, int error) {
    fprintf(stderr, "%s: %s\n", text, strerror(error));
}

int main() {
    pthread_t thread[NUM_THREADS];
    ThreadArgs_t threads_args[] = {
            {4, (char *[]) {
                    "T1 line 1/4",
                    "T1 line 2/4",
                    "T1 line 3/4",
                    "T1 line 4/4"}},
            {6, (char *[]) {
                    "T2 line 1/6",
                    "T2 line 2/6",
                    "T2 line 3/6",
                    "T2 line 4/6",
                    "T2 line 5/6",
                    "T2 line 6/6"}},
            {5, (char *[]) {
                    "T3 line 1/5",
                    "T3 line 2/5",
                    "T3 line 3/5",
                    "T3 line 4/5",
                    "T3 line 5/5"}},
            {3, (char *[]) {
                    "T4 line 1/3",
                    "T4 line 2/3",
                    "T4 line 3/3"}}
    };

    int error;
    for (int i = 0; i < NUM_THREADS; i++) {
        error = pthread_create(&thread[i], NULL, printLines, &threads_args[i]);
        if (error) {
            printError("Could not create thread", error);
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        error = pthread_join(thread[i], NULL);
        if (error) {
            printError("Could not join thread", error);
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
