#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* buf;
char** cmd;

void parse_line(char** buf, size_t* len, char** cmd);
bool handle_command(char** cmd);

void clean(void);
void handle_error(int ret) {
    if (ret == 0) return;
    fprintf(stderr, "error: %s\n", strerror(errno));
}

int main(void) {
    size_t len = 128;
    buf = malloc(len);
    cmd = malloc(sizeof(char*) * (_POSIX_ARG_MAX + 2));  // +2 for command name and termination symbol

    do {
        putchar('$');
        parse_line(&buf, &len, cmd);
    } while (handle_command(cmd));

    clean();
    return 0;
}

void parse_line(char** buf, size_t* len, char** cmd) {
    ssize_t nread;

    if ((nread = getline(buf, len, stdin)) < 0) {
        cmd[0] = NULL;
        return;
    }
    (*buf)[nread - 1] = '\0';  // remove '\n'

    if (nread == 1) {  // should distinguish an empty line and an EOL (Ctrl+D)
        cmd[0] = *buf;
        return;
    }

    int i = 0;
    cmd[i++] = strtok(*buf, " ");
    while ((cmd[i++] = strtok(NULL, " ")) != NULL);
}

bool handle_command(char** cmd) {
    if (cmd[0] == NULL || strcmp(cmd[0], "exit") == 0) {
        return false;
    }

    if (strcmp(cmd[0], "cd") == 0) {
        if (cmd[1] == NULL) {
            fprintf(stderr, "error: 'cd' requires 1 argument\n");
            return true;
        }
        handle_error(chdir(cmd[1]));
    } else if (strcmp(cmd[0], "history") == 0) {

    } else {

    }

    return true;
}

void clean(void) {
    free(buf);
    free(cmd);
}
