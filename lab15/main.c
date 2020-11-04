#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include<sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include<fcntl.h>

#define LINES_COUNT 10
#define UNLOCKED_SEMAPHORE 1
#define LOCKED_SEMAPHORE 0
#define CHILD_ID 0
#define ERROR -1

typedef struct {
    bool parent;
    sem_t *sem1;
    sem_t *sem2;
} Thread_parameters;

void printError(char *text, int error) {
    if (NULL == text) {
        return;
    }
    fprintf(stderr, "%s: %s\n", text, strerror(error));
}

int closeSemaphore(sem_t *sem_key, const char *sem_name) {
    int return_value = EXIT_SUCCESS;
    if (sem_key != NULL && ERROR == sem_close(sem_key)) {
        perror("sem_close");
        return_value = EXIT_FAILURE;
    }
    if (sem_name != NULL && ERROR == sem_unlink(sem_name)) {
        perror("sem_unlink");
        return_value = EXIT_FAILURE;
    }
    return return_value;
}


int printLines(void *args) {
    if (NULL == args) {
        fprintf(stderr, "Expected not null argument\n");
        return EXIT_FAILURE;
    }
    int error;
    Thread_parameters *parameters = (Thread_parameters *) args;
    char *name = (parameters->parent == true) ? "Parent" : "Child";
    sem_t *sem1 = parameters->sem1;
    sem_t *sem2 = parameters->sem2;

    for (int i = 1; i <= LINES_COUNT; i++) {
        error = sem_wait(sem1);
        if (error) {
            printError("Could not lock semaphore", error);
            return EXIT_FAILURE;
        }
        printf("%s thread: line %d\n", name, i);
        error = sem_post(sem2);
        if (error) {
            printError("Could not unlock semaphore", error);
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

int main() {
    Thread_parameters parameters;
    char *child_sem_name = "/unique_sem_name_1";
    char *parent_sem_name = "/unique_sem_name_2";
    sem_t *sem1, *sem2;
    int error;
    sem1 = sem_open(child_sem_name, O_CREAT | O_EXCL,
                    S_IRUSR | S_IWUSR, UNLOCKED_SEMAPHORE);
    if (SEM_FAILED == sem1) {
        perror("Failed to open semaphore");
        return EXIT_FAILURE;
    }
    sem2 = sem_open(parent_sem_name, O_CREAT | O_EXCL,
                    S_IRUSR | S_IWUSR, LOCKED_SEMAPHORE);
    if (SEM_FAILED == sem2) {
        perror("Failed to open semaphore");
        return EXIT_FAILURE;
    }
    pid_t child_pid;
    int return_value;
    child_pid = fork();
    if (child_pid == ERROR) {
        perror("Failed to fork process");
        return EXIT_FAILURE;
    } else if (child_pid == CHILD_ID) {
        parameters.sem1 = sem2;
        parameters.sem2 = sem1;
        parameters.parent = false;

    } else {
        parameters.sem1 = sem1;
        parameters.sem2 = sem2;
        parameters.parent = true;
    }

    return_value = printLines(&parameters);
    if (return_value == EXIT_FAILURE) {
        if (child_pid == CHILD_ID) {
            pid_t ppid = 0;
            ppid = getppid();
            if (ERROR == ppid) {
                perror("Failed to get parent process id");
            } else if (ERROR == kill(ppid, SIGKILL)) {
                perror("Failed to kill parent process");
            }
        } else {
            if (ERROR == kill(child_pid, SIGKILL)) {
                perror("Failed to kill child process");
            }
        }
    }

    if (child_pid == CHILD_ID) {
        if (EXIT_FAILURE == closeSemaphore(sem1, NULL)) {
            return_value = EXIT_FAILURE;
        }
        if (EXIT_FAILURE == closeSemaphore(sem2, NULL)) {
            return_value = EXIT_FAILURE;
        }
    } else {
        error = wait(NULL);
        if (error) {
            return_value = EXIT_FAILURE;
        }
        if (EXIT_FAILURE == closeSemaphore(sem1, child_sem_name)) {
            return_value = EXIT_FAILURE;
        }
        if (EXIT_FAILURE == closeSemaphore(sem2, parent_sem_name)) {
            return_value = EXIT_FAILURE;
        }
    }
    return return_value;
}
