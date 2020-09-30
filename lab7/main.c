#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define ITERATIONS_COUNT 200000000
#define NO_ERROR_CODE 0
#define MAX_THREADS_COUNT 100
#define ARGUMENTS_COUNT 2
#define NUMBER_ARGUMENT 1

void printError(char *text, int error) {
    if (NULL == text) {
        return;
    }
    if (NO_ERROR_CODE == error) {
        fprintf(stderr, "%s\n", text);
    } else {
        fprintf(stderr, "%s: %s\n", text, strerror(error));
    }
}

int getNumber(int *number, char *number_buffer) {
    if (NULL == number || NULL == number_buffer) {
        return EXIT_FAILURE;
    }
    char *end;
    size_t length = strlen(number_buffer);
    char *buffer_end = number_buffer + length;

    *number = strtol(number_buffer, &end, 10);
    if (buffer_end != end) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int getChunkSize(int count_chunks, int count_elements) {
    return (count_elements + count_chunks - 1) / count_chunks;
}

int getChunkStart(int chunk, int count_chunks, int count_elements) {
    int chunk_size = getChunkSize(count_chunks, count_elements);
    return chunk * chunk_size;
}

int getChunkEnd(int chunk, int count_chunks, int count_elements) {
    int chunk_size = getChunkSize(count_chunks, count_elements);
    int end = (chunk + 1) * chunk_size;
    if (end > count_elements) {
        return count_elements;
    } else {
        return end;
    }
}

typedef struct {
    int start;
    int end;
    double value;
} Chunk;

void *calculateChunk(void *args) {
    if (NULL == args) {
        return NULL;
    }
    Chunk *chunk = (Chunk *) args;
    double value = 0;
    double end = chunk->end;

    for (int i = chunk->start; i < end; i++) {
        value += 1.0 / (i * 4.0 + 1.0);
        value -= 1.0 / (i * 4.0 + 3.0);
    }
    chunk->value = value;
    return &(chunk->value);
}

int main(int argc, char **argv) {
    double pi = 0;
    int threads_count;
    if (ARGUMENTS_COUNT != argc) {
        printError("Required argument: number of threads", NO_ERROR_CODE);
        return EXIT_FAILURE;
    }
    int error = getNumber(&threads_count, argv[NUMBER_ARGUMENT]);
    if (EXIT_FAILURE == error) {
        printError("Wrong number format", NO_ERROR_CODE);
        return EXIT_FAILURE;
    }

    if (MAX_THREADS_COUNT < threads_count || 1 > threads_count) {
        fprintf(stderr, "Incorrect number of threads. You allowed to create 1 to %d threads\n",
                MAX_THREADS_COUNT);
        return EXIT_FAILURE;
    }

    pthread_t threads[threads_count];
    Chunk chunks[threads_count];

    //initialize chunks
    for (int i = 0; i < threads_count; i++) {
        chunks[i].start = getChunkStart(i, threads_count, ITERATIONS_COUNT);
        chunks[i].end = getChunkEnd(i, threads_count, ITERATIONS_COUNT);
        chunks[i].value = 0.0;
    }

    int working_threads_count = 0;
    int return_value = EXIT_SUCCESS;
    //create threads
    for (int i = 0; i < threads_count; i++) {
        error = pthread_create(&threads[i], NULL, calculateChunk, &chunks[i]);
        if (error) {
            return_value = EXIT_FAILURE;
            printError("Could not create threads", error);
            break;
        }
        working_threads_count++;
    }

    //collect values
    void *value;
    for (int i = 0; i < working_threads_count; i++) {
        error = pthread_join(threads[i], &value);
        if (error) {
            return_value = EXIT_FAILURE;
            printError("Could not join threads", error);
        }
        if (NULL == value) {
            return_value = EXIT_FAILURE;
            printError("Thread returned null pointer", NO_ERROR_CODE);
            continue;
        }
        pi += *(double *) value;
    }

    pi = pi * 4.0;
    printf("pi done: %.15g \n", pi);

    return return_value;
}
