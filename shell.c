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

void execute(struct command IO, int back_test) {
    pid_t child = fork();

    if (child == 0) {
        if (IO.outfile != NULL) {
            int out = open(IO.outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (out == -1) {
                perror("errno");
                exit(1);
            }
            dup2(out, 1);
        }
        if (IO.infile != NULL) {
            int in = open(IO.infile, O_RDONLY);
            if (in == -1) {
                perror("errno");
                exit(1);
            }
            dup2(in, 0);
        }
        execvp(IO.args[0], IO.args);
        perror("execvp");
        exit(1);
    } else if (child > 0) {
        if (back_test == 0) {
            waitpid(child, NULL, 0);
        }
    } else if (child < 0) {
        perror("fork");
    }
}

int main() {

    char *buffer = NULL;
    size_t size = 0;
    struct command IO;

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

        int back_task = 0;
        IO.infile = NULL;
        IO.outfile = NULL;

        while (token != NULL && index < 63) {
            if (strcmp(token, "&") == 0) {
                back_task = 1;
            } else if (strcmp(token, "~") == 0) {
                IO.args[index] = getenv("HOME");
                index++;
            } else if (strcmp(token, "<") == 0) {
                token = strtok_r(NULL, " ", &saveptr);
                IO.infile = token;
            } else if (strcmp(token, ">") == 0) {
                token = strtok_r(NULL, " ", &saveptr);
                IO.outfile = token;

            } else {
                IO.args[index] = token;
                index++;
            }
            token = strtok_r(NULL, " ", &saveptr);
        }
        IO.args[index] = NULL;

        if (IO.args[0] == NULL) {
            continue;
        }

        if (strcmp(IO.args[0], "cd") == 0) {
            if (IO.args[1] == NULL || strcmp(IO.args[1], "~") == 0) {
                chdir(getenv("HOME"));
            } else if (chdir(IO.args[1]) != 0) {
                perror("cd");
            }

            continue;
        }

        execute(IO, back_task);
    }

    free(buffer);
    return 0;
}
