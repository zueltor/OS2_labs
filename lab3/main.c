#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#define THREADS_COUNT 4

void printError(char *text, int error) {
    if (NULL == text) {
        return;
    }
    fprintf(stderr, "%s: %s\n", text, strerror(error));
}

typedef struct {
    int count;
    char **lines;
} ThreadArgs_t;

void *printLines(void *args) {
    if (NULL == args) {
        return NULL;
    }
    int num_lines = ((ThreadArgs_t *) args)->count;
    char **lines = ((ThreadArgs_t *) args)->lines;
    for (int i = 0; i < num_lines; i++) {
        printf("%s\n", lines[i]);
    }
    return NULL;
}

int main() {
    pthread_t thread[THREADS_COUNT];
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
    int working_threads_count = 0;
    int return_value = EXIT_SUCCESS;
    for (int i = 0; i < THREADS_COUNT; i++) {
        error = pthread_create(&thread[i], NULL, printLines, &threads_args[i]);
        if (error) {
            return_value = EXIT_FAILURE;
            printError("Could not create thread", error);
            break;
        }
        working_threads_count++;
    }

    for (int i = 0; i < working_threads_count; i++) {
        error = pthread_join(thread[i], NULL);
        if (error) {
            return_value = EXIT_FAILURE;
            printError("Could not join thread", error);
        }
    }
    return return_value;
}
