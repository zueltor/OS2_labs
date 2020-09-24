#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define printError(text, error) fprintf(stderr, text": %s\n",strerror(error));
#define NO_ERROR 0
#define THREADS_COUNT 10000

void *printLines(void *args) {
    sleep(4);
    return NULL;
}

int main() {
    pthread_t thread[THREADS_COUNT];
    int error = NO_ERROR;
    int return_value = EXIT_SUCCESS;
    int working_threads_count = 0;
    do {
        if (working_threads_count == THREADS_COUNT) {
            break;
        }
        error = pthread_create(&thread[working_threads_count], NULL, printLines, NULL);
        if (!error) {
            working_threads_count++;
        }
    } while (!error);
    if (error) {
        printError("Could not create thread", error);
    }
    for (int i = 0; i < working_threads_count; i++) {
        error = pthread_join(thread[i], NULL);
        if (error) {
            printError("Could not join thread", error);
            return_value = EXIT_FAILURE;
        }
    }

    printf("Threads created: %d\n", working_threads_count);

    return return_value;
}
