#include <fcntl.h>
#include <linux/limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

struct command {
    char *args[64];
    char *infile;
    char *outfile;
};

void execute(struct command IO[], int back_test, int cmd_index) {

    int index = 0;
    pid_t children[30];
    int prev_read = -1;

    while (index <= cmd_index) {

        int pipefd[2];

        if (pipe(pipefd) == -1) {
            perror("Pipe Error");
            exit(1);
        }

        children[index] = fork();

        if (children[index] == 0) { // child process
            if (prev_read != -1) {
                dup2(prev_read, 0);
                close(prev_read);
            }
            if (index < cmd_index) {
                dup2(pipefd[1], 1);
            }
            close(pipefd[0]);
            close(pipefd[1]);
            if (IO[index].outfile != NULL) {
                int out =
                    open(IO[index].outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (out == -1) {
                    perror("errno");
                    exit(1);
                }
                dup2(out, 1);
            }
            if (IO[index].infile != NULL) {
                int in = open(IO[index].infile, O_RDONLY);
                if (in == -1) {
                    perror("errno");
                    exit(1);
                }
                dup2(in, 0);
            }

            execvp(IO[index].args[0], IO[index].args);

            perror("execvp");
            exit(1);
        } else if (children[index] < 0) { // fork error
            perror("fork");
            exit(1);
        }
        if (prev_read != -1) {
            close(prev_read);
        }
        close(pipefd[1]);
        prev_read = pipefd[0];
        index++;
    }
    close(prev_read);
    for (int i = 0; i < index; i++) {
        if (back_test == 0) {
            waitpid(children[i], NULL, 0);
        }
    };
}

int main() {

    char *buffer = NULL;
    size_t size = 0;
    struct command IO[16];
    while (1) {

        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;

        char cwd[PATH_MAX];
        printf("Shell %s>", getcwd(cwd, sizeof(cwd)));
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
        int command_index = 0;

        int back_task = 0;
        IO[command_index].infile = NULL;
        IO[command_index].outfile = NULL;

        while (token != NULL && index < 63) {
            if (strcmp(token, "&") == 0) {
                back_task = 1;
            } else if (strcmp(token, "~") == 0) {
                IO[command_index].args[index] = getenv("HOME");
                index++;
            } else if (strcmp(token, "<") == 0) {
                token = strtok_r(NULL, " ", &saveptr);
                IO[command_index].infile = token;
            } else if (strcmp(token, ">") == 0) {
                token = strtok_r(NULL, " ", &saveptr);
                IO[command_index].outfile = token;
            } else if (strcmp(token, "|") == 0) {
                IO[command_index].args[index++] = NULL;
                command_index++;

                IO[command_index].infile = NULL;
                IO[command_index].outfile = NULL;

                index = 0;

            } else {
                IO[command_index].args[index] = token;
                index++;
            }
            token = strtok_r(NULL, " ", &saveptr);
        }
        IO[command_index].args[index] = NULL;

        if (IO[command_index].args[0] == NULL) {
            continue;
        }

        if (strcmp(IO[command_index].args[0], "cd") == 0) {
            if (IO[command_index].args[1] == NULL ||
                strcmp(IO[command_index].args[1], "~") == 0) {
                chdir(getenv("HOME"));
            } else if (chdir(IO[command_index].args[1]) != 0) {
                perror("cd");
            }

            continue;
        }

        execute(IO, back_task, command_index);
    }

    free(buffer);
    return 0;
}
