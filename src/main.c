#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern int errno;

static struct {
    char* bufs[10];
    size_t sizes[10];
    int idx;
    int last_id;
} history;

static char* buf;
static int buf_size = 128;

static char** cmd;

static bool bufs_ready = false;

void parseLine(char** buf, size_t* len, char** cmd);
bool handleCommand(char** cmd);
void historyCmd(char** args);

static void sigintHandler(int signum);
void clean(void);
void handleError(char* msg, bool should_exit);

int main(void) {
    struct sigaction sa;

    sa.sa_handler = sigintHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) < 0) handleError(NULL, true);

    for (int i = 0; i < 10; i++) {
        history.bufs[i] = malloc(buf_size);
        if (!history.bufs[i]) handleError(NULL, true);
        history.sizes[i] = buf_size;
    }

    buf = malloc(buf_size);
    if (!buf) handleError(NULL, true);

    cmd = malloc(sizeof(char*) * (_POSIX_ARG_MAX + 2));  // +2 for command name and termination symbol
    if (!cmd) handleError(NULL, true);

    bufs_ready = true;

    history.idx = -1;
    history.last_id = 0;

    do {
        if (putchar('$') == EOF) handleError("putchar", false);
        history.idx = (history.idx + 1) % 10;
        history.last_id++;
        parseLine(&history.bufs[history.idx], &history.sizes[history.idx], cmd);
    } while (handleCommand(cmd));

    clean();
    return 0;
}

void parseLine(char** hist_buf, size_t* size, char** cmd) {
    ssize_t nread;

    if ((nread = getline(hist_buf, size, stdin)) < 0) {
        cmd[0] = NULL;
        return;
    }
    (*hist_buf)[nread - 1] = '\0';  // remove '\n'

    // should not save an empty line or a duplicate command in history
    if (nread == 1 || strcmp(*hist_buf, history.bufs[(history.idx + 9) % 10]) == 0) {
        history.idx = (history.idx + 9) % 10;
        history.last_id--;
    }

    // should distinguish an empty line and an EOL (Ctrl+D)
    if (nread == 1) {
        cmd[0] = *hist_buf;
        return;
    }

    if (buf_size < *size) {
        free(buf);
        buf = malloc(*size);
        buf_size = *size;
    }
    memcpy(buf, *hist_buf, nread);

    int i = 0;
    cmd[i++] = strtok(buf, " ");
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
        historyCmd(cmd + 1);
    } else {
        switch(fork()) {
        case -1:
            handleError(NULL, true);
        case 0:
            if (execvp(cmd[0], cmd) < 0) handleError(NULL, false);
        default:
            if (wait(NULL) < 0) handleError(NULL, false);
        }
    }

    return true;
}

void historyCmd(char** args) {
    int print_count;

    if (args[0] == NULL) {
        print_count = 10;
    } else if (strcmp(args[0], "-c") == 0) {
        history.idx = -1;
        history.last_id = 0;
        return;
    } else {
        print_count = strtol(args[0], NULL, 10);
        if (errno == EINVAL || errno == ERANGE) {
            handleError(NULL, false);
            return;
        }
        if (print_count > 10) print_count = 10;
    }

    int id = 1;
    int idx = 0;

    if (history.last_id > print_count) {
        id = history.last_id - print_count + 1;
        idx = (history.idx - print_count + 11) % 10;
    }

    for (; id <= history.last_id; id++, idx = (idx + 1) % 10) {
        printf("%*d  %s\n", 5, id, history.bufs[idx]);
    }
}

static void sigintHandler(int signum) {
    if (bufs_ready) clean();
    exit(1);
}

void clean(void) {
    for (int i = 0; i < 10; i++) {
        free(history.bufs[i]);
    }
    free(buf);
    free(cmd);
}

void handleError(char* msg, bool should_exit) {
    if (!msg) msg = strerror(errno);
    fprintf(stderr, "error: %s\n", msg);
    if (should_exit) exit(1);
}
