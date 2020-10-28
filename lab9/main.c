#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define PHILOSOPHERS_COUNT 5
#define THINK_TIME 20000000
#define EAT_TIME 40000000
#define FOOD 50
#define NANOSECONDS_IN_SECOND 1000000000ll
#define RANDOM_VARIANCE_MULTIPLIER 3
#define FOOD_LEFT_ON_TABLE 1
//#define SLOWPOKE 1

typedef struct {
    int id;
    pthread_mutex_t *fork1;
    pthread_mutex_t *fork2;
    pthread_mutex_t *foodlock;
} Philosopher_parameters;

int min(int a, int b) {
    return (a < b) ? a : b;
}

int max(int a, int b) {
    return (a < b) ? b : a;
}

void printError(char *text, int error) {
    if (NULL == text) {
        return;
    }
    fprintf(stderr, "%s: %s\n", text, strerror(error));
}

void getForks(Philosopher_parameters *philosopher_parameters) {
    if (NULL == philosopher_parameters) {
        fprintf(stderr, "Parameters cannot be null\n");
        return;
    }
    int error;
    error = pthread_mutex_lock(philosopher_parameters->fork1);
    if (error) {
        printError("Mutex lock error", error);
        exit(EXIT_FAILURE);
    }
    printf("Philosopher %d: got 1st fork\n", philosopher_parameters->id);
    error = pthread_mutex_lock(philosopher_parameters->fork2);
    if (error) {
        printError("Mutex lock error", error);
        exit(EXIT_FAILURE);
    }
    printf("Philosopher %d: got 2nd fork\n", philosopher_parameters->id);
}

void putDownForks(Philosopher_parameters *philosopher_parameters) {
    if (NULL == philosopher_parameters) {
        fprintf(stderr, "Parameters cannot be null\n");
        return;
    }
    int error;
    error = pthread_mutex_unlock(philosopher_parameters->fork2);
    if (error) {
        printError("Mutex unlock error", error);
        exit(EXIT_FAILURE);
    }
    error = pthread_mutex_unlock(philosopher_parameters->fork1);
    if (error) {
        printError("Mutex unlock error", error);
        exit(EXIT_FAILURE);
    }
}

void doSleep(long long sleep_time_nanoseconds) {
    long long _seconds = sleep_time_nanoseconds / NANOSECONDS_IN_SECOND;
    long long _nanoseconds = sleep_time_nanoseconds % NANOSECONDS_IN_SECOND;
    int seconds = (int) _seconds;
    int nanoseconds = (int) _nanoseconds;
    struct timespec sleep_time = {seconds, nanoseconds};
    nanosleep(&sleep_time, NULL);
}

void think(int id) {
    double variance = (double) rand() / RAND_MAX * (THINK_TIME * RANDOM_VARIANCE_MULTIPLIER);
    long long think_time = (long long) (THINK_TIME + variance) * (id + 1);
    printf("%d think %llu\n",id,think_time);
    /*if (id == SLOWPOKE) {
        think_time *= 30;
    }*/
    doSleep(think_time);
}

void eat(int id) {
    printf("Philosopher %d: eating.\n", id);
    double variance = (double) rand() / RAND_MAX * (EAT_TIME * RANDOM_VARIANCE_MULTIPLIER);
    long long eat_time = (long long) (EAT_TIME + variance) * (id + 1);
    /*if (id == SLOWPOKE) {
        eat_time *= 8;
    }*/
    doSleep(eat_time);
}

int takeFood(Philosopher_parameters *philosopher_parameters) {
    if (NULL == philosopher_parameters) {
        fprintf(stderr, "Parameters cannot be null\n");
        return 0;
    }
    static int food = FOOD + 1;
    int myfood;
    int error;
    error = pthread_mutex_lock(philosopher_parameters->foodlock);
    if (error) {
        printError("Mutex lock error", error);
        exit(EXIT_FAILURE);
    }
    if (food > 0) {
        food--;
    }
    myfood = food;
    error = pthread_mutex_unlock(philosopher_parameters->foodlock);
    if (error) {
        printError("Mutex unlock error", error);
        exit(EXIT_FAILURE);
    }
    return myfood;
}

void *startDinner(void *args) {
    int id;
    int food_left;
    Philosopher_parameters *philosopher_parameters = (Philosopher_parameters *) args;
    id = philosopher_parameters->id;
    static __thread int count = 0;
    printf("Philosopher %d sitting down to dinner.\n", id);

    while (FOOD_LEFT_ON_TABLE) {
        think(id);
        getForks(philosopher_parameters);
        food_left = takeFood(philosopher_parameters);
        if (!food_left) {
            putDownForks(philosopher_parameters);
            printf("Philosopher %d is done eating, he ate %d times\n", id, count);
            break;
        }
        eat(id);
        count++;
        putDownForks(philosopher_parameters);
    }
    return (NULL);
}

int main(int argn, char **argv) {
    srand(time(NULL));

    int error;
    pthread_mutex_t forks[PHILOSOPHERS_COUNT];
    pthread_mutex_t foodlock = PTHREAD_MUTEX_INITIALIZER;
    pthread_t philosophers[PHILOSOPHERS_COUNT];
    Philosopher_parameters philosopher_parameters[PHILOSOPHERS_COUNT];

    for (int i = 0; i < PHILOSOPHERS_COUNT; i++) {
        pthread_mutex_t mutex_intializer = PTHREAD_MUTEX_INITIALIZER;
        forks[i] = mutex_intializer;
    }

    for (int index = 0; index < PHILOSOPHERS_COUNT; index++) {
        philosopher_parameters[index].id = index;
        int next_index = (index + 1) % PHILOSOPHERS_COUNT;
        philosopher_parameters[index].fork1 = &forks[min(index, next_index)];
        philosopher_parameters[index].fork2 = &forks[max(index, next_index)];
        philosopher_parameters[index].foodlock = &foodlock;
    }

    int philosophers_count = 0;
    int return_value = EXIT_SUCCESS;
    for (int i = 0; i < PHILOSOPHERS_COUNT; i++) {
        error = pthread_create(&philosophers[i], NULL, startDinner, &philosopher_parameters[i]);
        if (error) {
            printError("Philosopher could not come to the table", error);
            return_value = EXIT_FAILURE;
            break;
        }
        philosophers_count++;
    }
    for (int i = 0; i < philosophers_count; i++) {
        error = pthread_join(philosophers[i], NULL);
        if (error) {
            printError("Could not join thread", error);
            return_value = EXIT_FAILURE;
        }
    }
    return return_value;
}
