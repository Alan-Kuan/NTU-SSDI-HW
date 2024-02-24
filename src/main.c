#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern int errno;

char* buf;
char** cmd;

void parseLine(char** buf, size_t* len, char** cmd);
bool handleCommand(char** cmd);

void clean(void);
void handleError(char* msg, bool should_exit);

int main(void) {
    size_t len = 128;

    buf = malloc(len);
    if (!buf) handleError(NULL, true);

    cmd = malloc(sizeof(char*) * (_POSIX_ARG_MAX + 2));  // +2 for command name and termination symbol
    if (!cmd) handleError(NULL, true);

    do {
        if (putchar('$') == EOF) handleError("putchar", false);
        parseLine(&buf, &len, cmd);
    } while (handleCommand(cmd));

    clean();
    return 0;
}

void parseLine(char** buf, size_t* len, char** cmd) {
    ssize_t nread;

    if ((nread = getline(buf, len, stdin)) < 0) {
        cmd[0] = NULL;
        return;
    }
    (*buf)[nread - 1] = '\0';  // remove '\n'

    // should distinguish an empty line and an EOL (Ctrl+D)
    if (nread == 1) {
        cmd[0] = *buf;
        return;
    }

    int i = 0;
    cmd[i++] = strtok(*buf, " ");
    while ((cmd[i++] = strtok(NULL, " ")) != NULL);
}

bool handleCommand(char** cmd) {
    if (cmd[0] == NULL || strcmp(cmd[0], "exit") == 0) {
        return false;
    }

    if (strcmp(cmd[0], "cd") == 0) {
        if (cmd[1] == NULL) {
            fprintf(stderr, "error: 'cd' requires 1 argument\n");
            return true;
        }
        if (chdir(cmd[1]) < 0) handleError(NULL, false);
    } else if (strcmp(cmd[0], "history") == 0) {

    } else {

    }

    return true;
}

void clean(void) {
    free(buf);
    free(cmd);
}

void handleError(char* msg, bool should_exit) {
    if (!msg) msg = strerror(errno);
    fprintf(stderr, "error: %s\n", msg);
    if (should_exit) exit(1);
}
