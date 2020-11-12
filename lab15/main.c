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

typedef void (*signalHandler)(int);

int signalReceived = false;

typedef struct {
    bool parent;
    sem_t *wait_semaphore;
    sem_t *post_semaphore;
} Thread_parameters;

int closeSemaphore(sem_t *sem_key, const char *sem_name) {
    int return_value = EXIT_SUCCESS;
    if (sem_key != NULL && ERROR == sem_close(sem_key)) {
        perror("Failed to close semaphore");
        return_value = EXIT_FAILURE;
    }
    if (sem_name != NULL && ERROR == sem_unlink(sem_name)) {
        perror("Failed to unlink semaphore");
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
    sem_t *wait_semaphore = parameters->wait_semaphore;
    sem_t *post_semaphore = parameters->post_semaphore;

    for (int i = 1; i <= LINES_COUNT && !signalReceived; i++) {
        error = sem_wait(wait_semaphore);
        if (error) {
            perror("Could not lock semaphore");
            return EXIT_FAILURE;
        }
#ifdef SLEEP
        sleep(1);
#endif
        printf("%s process: line %d\n", name, i);
        error = sem_post(post_semaphore);
        if (error) {
            perror("Could not unlock semaphore");
            return EXIT_FAILURE;
        }
    }

    return (signalReceived) ? EXIT_FAILURE : EXIT_SUCCESS;
}

void stopExecution(int signal) {
    signalReceived = true;
}

int main() {
    Thread_parameters parameters;
    char *child_semaphore_name = "/unique_child_semaphore_name";
    char *parent_semaphore_name = "/unique_parent_semaphore_name";
    sem_t *parent_semaphore, *child_semaphore;
    int error;
    bool executionError = false;
    bool process_killed = false;
    parent_semaphore = sem_open(child_semaphore_name, O_CREAT | O_EXCL,
                                S_IRUSR | S_IWUSR, UNLOCKED_SEMAPHORE);
    if (SEM_FAILED == parent_semaphore) {
        perror("Failed to open semaphore");
        return EXIT_FAILURE;
    }
    child_semaphore = sem_open(parent_semaphore_name, O_CREAT | O_EXCL,
                               S_IRUSR | S_IWUSR, LOCKED_SEMAPHORE);
    if (SEM_FAILED == child_semaphore) {
        perror("Failed to open semaphore");
        closeSemaphore(parent_semaphore, parent_semaphore_name);
        return EXIT_FAILURE;
    }
    pid_t child_pid;
    int return_value = EXIT_SUCCESS;

    signalHandler prevHandler = signal(SIGINT, stopExecution);
    if (prevHandler == SIG_ERR) {
        closeSemaphore(parent_semaphore, parent_semaphore_name);
        closeSemaphore(child_semaphore, child_semaphore_name);
        perror("Could not set signal handler");
        return EXIT_FAILURE;
    }

    child_pid = fork();
    if (child_pid == ERROR) {
        perror("Failed to fork process");
        return EXIT_FAILURE;
    } else if (child_pid == CHILD_ID) {
        parameters.wait_semaphore = child_semaphore;
        parameters.post_semaphore = parent_semaphore;
        parameters.parent = false;
    } else {
        parameters.wait_semaphore = parent_semaphore;
        parameters.post_semaphore = child_semaphore;
        parameters.parent = true;
    }

    return_value = printLines(&parameters);
    if ((return_value == EXIT_FAILURE) && !signalReceived) {
        if (child_pid == CHILD_ID) {
            pid_t ppid = 0;
            ppid = getppid();
            printf("Sending kill to parent\n");
            if (ERROR == ppid) {
                perror("Failed to get parent process id");
            } else if (ERROR == kill(ppid, SIGKILL)) {
                perror("Failed to kill parent process");
            } else {
                process_killed = true;
            }
        } else {
            printf("Sending kill to child\n");
            if (ERROR == kill(child_pid, SIGKILL)) {
                perror("Failed to kill child process");
            } else {
                process_killed = true;
            }
        }
    }
    if (process_killed) {
        if (child_pid == CHILD_ID) {
            printf("Parent died\n");
        } else {
            printf("Child died\n");
        }
    }
    if (child_pid == CHILD_ID) {
        char *semaphore_name = (process_killed) ? parent_semaphore_name : NULL;
        if (EXIT_FAILURE == closeSemaphore(parent_semaphore, semaphore_name)) {
            return_value = EXIT_FAILURE;
        }
        semaphore_name = (process_killed) ? child_semaphore_name : NULL;
        if (EXIT_FAILURE == closeSemaphore(child_semaphore, semaphore_name)) {
            return_value = EXIT_FAILURE;
        }
    } else {
        error = wait(NULL);
        if (error) {
            return_value = EXIT_FAILURE;
        }
        if (EXIT_FAILURE == closeSemaphore(parent_semaphore, child_semaphore_name)) {
            return_value = EXIT_FAILURE;
        }
        if (EXIT_FAILURE == closeSemaphore(child_semaphore, parent_semaphore_name)) {
            return_value = EXIT_FAILURE;
        }
    }
    printf("Closed semaphores\n");
    return (executionError) ? EXIT_FAILURE : return_value;
}
