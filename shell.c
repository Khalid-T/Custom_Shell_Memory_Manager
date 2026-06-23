#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {

    char *buffer = NULL;
    size_t size = 0;
    while (1) {
        printf("sHELL>");
        if (getline(&buffer, &size, stdin) == -1) {
            break; // EOF (Ctrl-D) or error
        }
        buffer[strcspn(buffer, "\n")] = '\0'; // strip the newline
        if (strcmp(buffer, "exit") == 0) {
            break;
        }
    }
    free(buffer);
    return 0;
}
