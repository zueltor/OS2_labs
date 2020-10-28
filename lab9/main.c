#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define PHILOSOPHERS_COUNT 5
#define THINK_TIME 30000000
#define EAT_TIME 100000000
#define FOOD 50
#define NANOSECONDS_IN_SECOND 1000000000
#define RANDOM_VARIANCE_MULTIPLIER 3
#define FOOD_LEFT_ON_TABLE 1

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

void doSleep(int sleep_time_nanoseconds) {
    int seconds = sleep_time_nanoseconds / NANOSECONDS_IN_SECOND;
    int nanoseconds = sleep_time_nanoseconds % NANOSECONDS_IN_SECOND;
    struct timespec sleep_time = {seconds, nanoseconds};
    nanosleep(&sleep_time, NULL);
}

void think(int id) {
    int think_time = THINK_TIME * (id + 1) + rand() % (THINK_TIME * RANDOM_VARIANCE_MULTIPLIER * (id + 1));
    doSleep(think_time);
}

void eat(int id) {
    printf("Philosopher %d: eating.\n", id);
    int eat_time = EAT_TIME * (id + 1) + rand() % (EAT_TIME * RANDOM_VARIANCE_MULTIPLIER * (id + 1));
    doSleep(eat_time);
}

int takeFood(Philosopher_parameters *philosopher_parameters) {
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


int destroyResources(pthread_mutex_t mutexes, int count) {
    int error;
    int return_value = EXIT_SUCCESS;
    for (int i = 0; i < count; i++) {
        error = pthread_mutexattr_destroy(&mutexes[i]);
        if (error) {
            return_value = EXIT_FAILURE;
            printError("Could not destroy mutex", error);
        }
    }
    return return_value;
}

int main(int argn, char **argv) {
    srand(time(NULL));

    int error;
    pthread_mutex_t forks[PHILOSOPHERS_COUNT];
    pthread_mutex_t foodlock;
    pthread_t philosophers[PHILOSOPHERS_COUNT];
    Philosopher_parameters philosopher_parameters[PHILOSOPHERS_COUNT];

    error = pthread_mutex_init(&foodlock, NULL);
    if (error) {
        printError("Could not init mutex", error);
        destroyResources(forks, 1);
        return EXIT_FAILURE;
    }
    for (int i = 0; i < PHILOSOPHERS_COUNT; i++) {
        error = pthread_mutex_init(&forks[i], NULL);
        if (error) {
            printError("Could not init mutex", error);
            destroyResources(forks, i + 1);
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < PHILOSOPHERS_COUNT; i++) {
        philosopher_parameters[i].id = i;
        philosopher_parameters[i].fork1 = &forks[min(i, (i + 1) % PHILOSOPHERS_COUNT)];
        philosopher_parameters[i].fork2 = &forks[max(i, (i + 1) % PHILOSOPHERS_COUNT)];
        philosopher_parameters[i].foodlock = &foodlock;
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

    error = destroyResources(forks, PHILOSOPHERS_COUNT);
    if (error != EXIT_SUCCESS) {
        return_value = EXIT_FAILURE;
    }
    error = destroyResources(foodlock, 1);
    if (error != EXIT_SUCCESS) {
        return_value = EXIT_FAILURE;
    }
    return return_value;
}



