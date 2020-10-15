#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>
#include <stdbool.h>

#define ITERATIONS_CHUNK_COUNT 2882880
#define NO_ERROR_CODE 0
#define MAX_THREADS_COUNT 1000
#define ARGUMENTS_COUNT 2
#define THREADS_COUNT_ARGUMENT 1
#define NO_STOP_SIGNAL 1
#define FIRST_THREAD (start % ITERATIONS_CHUNK_COUNT == 0)
#define NO_ERROR 0

volatile sig_atomic_t stop_signal = false;

typedef void (*signalHandler)(int);

typedef struct {
    unsigned start;
    unsigned end;
    double value;
    pthread_barrier_t *barrier_p;
} Chunk;

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

unsigned getChunkSize(unsigned count_chunks, unsigned count_elements) {
    return (count_elements + count_chunks - 1) / count_chunks;
}

unsigned getChunkStart(unsigned chunk, unsigned count_chunks, unsigned count_elements) {
    unsigned chunk_size = getChunkSize(count_chunks, count_elements);
    return chunk * chunk_size;
}

unsigned getChunkEnd(unsigned chunk, unsigned count_chunks, unsigned count_elements) {
    unsigned chunk_size = getChunkSize(count_chunks, count_elements);
    unsigned end = (chunk + 1) * chunk_size;
    if (end > count_elements) {
        return count_elements;
    } else {
        return end;
    }
}

void *calculateChunk(void *args) {
    if (NULL == args) {
        return NULL;
    }
    static bool stop_calculating = false;
    int error;
    sigset_t mask;
    error = sigemptyset(&mask);
    if (error) {
        printError("Could not sig empty set", error);
        stop_calculating = true;
    }
    error = sigaddset(&mask, SIGINT);
    if (error) {
        printError("Could not sig empty set", error);
        stop_calculating = true;
    }
    error = pthread_sigmask(SIG_BLOCK, &mask, NULL);
    if (error) {
        printError("Could not sig empty set", error);
        stop_calculating = true;
    }
    Chunk *chunk = (Chunk *) args;
    double value = 0;
    unsigned long long end = chunk->end;
    unsigned long prev_end;
    unsigned long long start = chunk->start;

    pthread_barrier_t *barrier_p = chunk->barrier_p;

    while (NO_STOP_SIGNAL) {
        for (unsigned long long i = start; i < end; i++) {
            value += 1.0 / (i * 4.0 + 1.0);
            value -= 1.0 / (i * 4.0 + 3.0);
        }
        prev_end = end;
        start += ITERATIONS_CHUNK_COUNT;
        end += ITERATIONS_CHUNK_COUNT;
        if (prev_end > end) {
            printf("Overflow occurred\n");
            stop_calculating = true;
        }
        if (FIRST_THREAD) {
            if (stop_signal) {
                stop_calculating = true;
            }
        }
        error = pthread_barrier_wait(barrier_p);
        if (PTHREAD_BARRIER_SERIAL_THREAD != error && NO_ERROR != error) {
            printError("Barrier1 wait error", error);
            exit(EXIT_FAILURE);
        }
        if (stop_calculating) {
            break;
        }
        error = pthread_barrier_wait(barrier_p);
        if (PTHREAD_BARRIER_SERIAL_THREAD != error && NO_ERROR != error) {
            printError("Barrier2 wait error", error);
            exit(EXIT_FAILURE);
        }
    }
    chunk->value = value;
    return &(chunk->value);
}

void stopExecution(int signal) {
    if (signal != SIGINT) {
        printf("Handled unknown signal\n");
    }
    stop_signal = true;
}

int main(int argc, char **argv) {
    double pi = 0;
    int return_value = EXIT_SUCCESS;
    int threads_count;

    if (ARGUMENTS_COUNT != argc) {
        printError("Required argument: number of threads", NO_ERROR_CODE);
        return EXIT_FAILURE;
    }

    int error = getNumber(&threads_count, argv[THREADS_COUNT_ARGUMENT]);
    if (EXIT_FAILURE == error) {
        printError("Wrong number format", NO_ERROR_CODE);
        return EXIT_FAILURE;
    }

    if (MAX_THREADS_COUNT < threads_count || 1 > threads_count) {
        fprintf(stderr, "Incorrect number of threads. You allowed to create 1 to %d threads\n",
                MAX_THREADS_COUNT);
        return EXIT_FAILURE;
    }

    pthread_barrier_t barrier;
    error = pthread_barrier_init(&barrier, NULL, threads_count);
    if (error) {
        printError("Could not initialize barrier", error);
        return EXIT_FAILURE;
    }

    //setting signal handler
    signalHandler prevHandler = signal(SIGINT, stopExecution);
    if (prevHandler == SIG_ERR) {
        perror("Could not set signal handler");
        error = pthread_barrier_destroy(&barrier);
        if (error) {
            printError("Could not destroy barrier", error);
        }
        return EXIT_FAILURE;
    }
    pthread_t threads[threads_count];
    Chunk chunks[threads_count];

    //initialize chunks
    for (int i = 0; i < threads_count; i++) {
        chunks[i].start = getChunkStart(i, threads_count, ITERATIONS_CHUNK_COUNT);
        chunks[i].end = getChunkEnd(i, threads_count, ITERATIONS_CHUNK_COUNT);
        chunks[i].value = 0.0;
        chunks[i].barrier_p = &barrier;
    }

    //create threads
    int working_threads_count = 0;
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
    printf("\nCalculated pi: \t%.15g \nReal pi: \t%.15g\n", pi, M_PI);

    error = pthread_barrier_destroy(&barrier);
    if (error) {
        return_value = EXIT_FAILURE;
        printError("Could not destroy barrier", error);
    }
    return return_value;
}
