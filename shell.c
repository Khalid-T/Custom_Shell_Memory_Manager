#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void execute(char **args, int back_task) {

    pid_t child = fork();

    if (child == 0) {
        execvp(args[0], args);
        perror("execvp");
        exit(1);
    } else if (child > 0) {
        if (back_task == 0) {
            waitpid(child, NULL, 0);
        }
    } else if (child < 0) {
        perror("fork");
    }
}

int main() {

    char *buffer = NULL;
    size_t size = 0;

    char *args[64];
    while (1) {
        waitpid(-1, NULL, WNOHANG);
        printf("sHELL>");
        if (getline(&buffer, &size, stdin) == -1) {
            break;
        }
        buffer[strcspn(buffer, "\n")] = '\0';
        if (strcmp(buffer, "exit") == 0) {
            break;
        }
        int index = 0;
        char *saveptr;

        char *token = strtok_r(buffer, " ", &saveptr);
        int back_task = 0;

        while (token != NULL && index < 63) {
            if (strcmp(token, "&") != 0) {
                args[index] = token;
                index++;
            } else {
                back_task = 1;
            }
            token = strtok_r(NULL, " ", &saveptr);
        }
        args[index] = NULL;

        if (args[0] == NULL) {
            continue;
        }

        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                chdir(getenv("HOME"));
            } else if (chdir(args[1]) != 0) {
                perror("cd");
            }

            continue;
        }

        execute(args, back_task);
    }

    free(buffer);
    return 0;
}
