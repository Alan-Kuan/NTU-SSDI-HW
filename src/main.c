#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* buf;
char** cmd;

void parse_line(char** buf, size_t* len, char** cmd);
bool handle_command(char** cmd);

void clean(void);

int main(void) {
    size_t len = 128;
    buf = malloc(len);
    cmd = malloc(sizeof(char*) * (_POSIX_ARG_MAX + 2));  // +2 for command name and termination symbol

    do {
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

    int i = 0;
    cmd[i++] = strtok(*buf, " ");
    while ((cmd[i++] = strtok(NULL, " ")) != NULL);
}

bool handle_command(char** cmd) {
    if (!cmd[0]) {
        return false;
    }

    return true;
}

void clean(void) {
    free(buf);
    free(cmd);
}
