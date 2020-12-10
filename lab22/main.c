#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>

#define DETAIL_A 0
#define DETAIL_A_CREATE_TIME 1
#define DETAIL_A_RESOURCES_COUNT 0

#define DETAIL_B 1
#define DETAIL_B_CREATE_TIME 2
#define DETAIL_B_RESOURCES_COUNT 0

#define DETAIL_C 2
#define DETAIL_C_CREATE_TIME 3
#define DETAIL_C_RESOURCES_COUNT 0

#define MODULE 3
#define MODULE_CREATE_TIME 0
#define MODULE_RESOURCES_COUNT 2

#define WIDGET 4
#define WIDGET_CREATE_TIME 0
#define WIDGET_RESOURCES_COUNT 2

#define CREATORS_COUNT 5
#define SHARED_SEMAPHORE 0
#define NO_DETAILS_LIMIT 0
#define WIDGETS_TO_CREATE 5
#define DETAIL_CREATOR_ERROR (void*)-1

typedef struct {
    int create_time;
    char *detail_name;
    int details_to_create;
    sem_t *my_detail;
    sem_t *resources;
    int resources_count;
} Details_parameters;

void printError(char *text, int error) {
    if (NULL == text) {
        return;
    }
    fprintf(stderr, "%s: %s\n", text, strerror(error));
}

int allThreadsCreated(int working_threads_count,int threads_count){
    return working_threads_count==threads_count;
}

void *detailCreator(void *args) {
    if (NULL == args) {
        return NULL;
    }
    Details_parameters *parameters=(Details_parameters*)args;
    int create_time = parameters->create_time;
    char *detail_name = parameters->detail_name;
    sem_t *resources = parameters->resources;
    sem_t *my_detail = parameters->my_detail;
    int resources_count = parameters->resources_count;
    int details_to_create = parameters->details_to_create;
    int details_created = 0;
    int error;
    while (details_created < details_to_create || details_to_create == NO_DETAILS_LIMIT) {
        for (int i = 0; i < resources_count; i++) {
            error = sem_wait(&resources[i]);
            if (error) {
                return DETAIL_CREATOR_ERROR;
            }
        }
        unsigned int unslept_time = create_time;
        while (unslept_time > 0) {
            unslept_time = sleep(unslept_time);
        };
        error = sem_post(my_detail);
        if (error) {
            return DETAIL_CREATOR_ERROR;
        }
        details_created++;
        printf("%s created, total created: %d\n", detail_name, details_created);
    }
    return NULL;
}

void destroySemaphores(sem_t *semaphores, int count) {
    int error;
    for (int i = 0; i < count; i++) {
        error = sem_destroy(&semaphores[i]);
        if (error) {
            printError("Could not destroy semaphores", error);
        }
    }
}

int main() {
    pthread_t thread[CREATORS_COUNT];
    int error;
    sem_t resources[CREATORS_COUNT];
    int resources_count = 0;

    for (int i = 0; i < CREATORS_COUNT; i++) {
        error = sem_init(&resources[i], SHARED_SEMAPHORE, 0);
        if (error) {
            printError("Could not init semaphore", error);
            destroySemaphores(resources, resources_count);
            return EXIT_FAILURE;
        }
        resources_count++;
    }

    Details_parameters threads_args[] = {
            {DETAIL_A_CREATE_TIME, "Detail A", NO_DETAILS_LIMIT,  &resources[DETAIL_A], resources,            DETAIL_A_RESOURCES_COUNT},
            {DETAIL_B_CREATE_TIME, "Detail B", NO_DETAILS_LIMIT,  &resources[DETAIL_B], resources,            DETAIL_B_RESOURCES_COUNT},
            {DETAIL_C_CREATE_TIME, "Detail C", NO_DETAILS_LIMIT,  &resources[DETAIL_C], resources,            DETAIL_C_RESOURCES_COUNT},
            {MODULE_CREATE_TIME,   "Module",   NO_DETAILS_LIMIT,  &resources[MODULE],   resources,            MODULE_RESOURCES_COUNT},
            {WIDGET_CREATE_TIME,   "Widget",   WIDGETS_TO_CREATE, &resources[WIDGET],   &resources[DETAIL_C], WIDGET_RESOURCES_COUNT}
    };

    int working_threads_count = 0;
    int return_value = EXIT_SUCCESS;
    for (int i = 0; i < CREATORS_COUNT; i++) {
        error = pthread_create(&thread[i], NULL, detailCreator, &threads_args[i]);
        if (error) {
            return_value = EXIT_FAILURE;
            printError("Could not create thread", error);
            break;
        }
        working_threads_count++;
    }

    void *result;

    //join widget thread
    if (allThreadsCreated(working_threads_count,CREATORS_COUNT)) {
        error = pthread_join(thread[WIDGET], &result);
        if (error || result == DETAIL_CREATOR_ERROR) {
            return_value = EXIT_FAILURE;
        }
        if (result != DETAIL_CREATOR_ERROR) {
            printf("All widgets created\n");
        }
        working_threads_count--;
    }

    //cancel other threads
    for (int i = 0; i < working_threads_count; i++) {
        error = pthread_cancel(thread[i]);
        if (error) {
            printError("Could not cancel thread", error);
            destroySemaphores(resources, resources_count);
            return EXIT_FAILURE;
        }
    }

    //join other threads
    for (int i = 0; i < working_threads_count; i++) {
        error = pthread_join(thread[i], &result);
        if (error) {
            return_value = EXIT_FAILURE;
            printError("Could not join thread", error);
        }
        if (result == DETAIL_CREATOR_ERROR) {
            return_value = EXIT_FAILURE;
        }
    }

    destroySemaphores(resources, resources_count);
    return return_value;
}
