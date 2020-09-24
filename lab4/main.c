#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define NOT_CANCELLED 1
#define SLEEP_TIME 2
#define printError(text, error) fprintf(stderr, text": %s\n",strerror(error));

void *foreverPrintLines(void *args) {
    int line_number = 1;
    while (NOT_CANCELLED) {
        printf("Line #%d\n", line_number++);
        pthread_testcancel();
    }
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
