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

static bool bufs_ready = false;

bool handleInput(char** buf, size_t* len);
bool handleCommand(void);
bool runCommand(char* cmd, int fds[], int fd_in);
void historyCmd(char** args, FILE* f_out);

static void sigintHandler(int signum);
void clean(void);
void handleError(char* msg, int should_exit);

int main(void) {
    struct sigaction sa;

    sa.sa_handler = sigintHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) < 0) handleError(NULL, 1);

    for (int i = 0; i < 10; i++) {
        history.bufs[i] = malloc(buf_size);
        if (!history.bufs[i]) handleError(NULL, 1);
        history.sizes[i] = buf_size;
    }

    buf = malloc(buf_size);
    if (!buf) handleError(NULL, 1);

    bufs_ready = true;

    history.idx = -1;
    history.last_id = 0;

    while (true) {
        if (putchar('$') == EOF) handleError("putchar", 0);
        history.idx = (history.idx + 1) % 10;
        history.last_id++;
        if (!handleInput(&history.bufs[history.idx], &history.sizes[history.idx])) {
            break;
        }
        if (!handleCommand()) {
            break;
        }
    }

    clean();
    return 0;
}

bool handleInput(char** hist_buf, size_t* size) {
    ssize_t nread;

    if ((nread = getline(hist_buf, size, stdin)) < 0) {
        return false;
    }
    (*hist_buf)[nread - 1] = '\0';  // remove '\n'

    // should not save an empty line or a duplicate command in history
    if (nread == 1 || strcmp(*hist_buf, history.bufs[(history.idx + 9) % 10]) == 0) {
        history.idx = (history.idx + 9) % 10;
        history.last_id--;
    }

    if (buf_size < *size) {
        free(buf);
        buf = malloc(*size);
        buf_size = *size;
    }
    memcpy(buf, *hist_buf, nread);

    return true;
}

bool handleCommand(void) {
    if (buf[0] == '\0') return true;

    char* saveptr_pipe;
    int fds[2];
    int fd_in = -1;

    char* cmd = strtok_r(buf, "|", &saveptr_pipe);
    char* cmd_next;
    while ((cmd_next = strtok_r(NULL, "|", &saveptr_pipe)) != NULL) {
        if (pipe(fds) < 0) handleError(NULL, 1);
        if (!runCommand(cmd, fds, fd_in)) {
            return false;
        }
        cmd = cmd_next;
        fd_in = fds[0];
    }
    return runCommand(cmd, NULL, fd_in);
}

bool runCommand(char* cmd, int fds[], int fd_in) {
    char* saveptr_space;
    char* argv[_POSIX_ARG_MAX + 2];  // +2 for command name and termination symbol
    int argv_len = 0;

    argv[argv_len++] = strtok_r(cmd, " ", &saveptr_space);
    while ((argv[argv_len++] = strtok_r(NULL, " ", &saveptr_space)) != NULL);

    if (strcmp(argv[0], "exit") == 0) {
        return false;
    } else if (strcmp(argv[0], "cd") == 0) {
        if (argv[1] == NULL) {
            fprintf(stderr, "error: 'cd' requires 1 argument\n");
            return true;
        }
        if (chdir(argv[1]) < 0) handleError(NULL, 0);
    } else if (strcmp(argv[0], "history") == 0) {
        if (fds) {
            FILE* f_out = fdopen(fds[1], "w");
            if (!f_out) handleError(NULL, 1);
            historyCmd(argv + 1, f_out);
            if (fclose(f_out) == EOF) handleError(NULL, 0);
        } else {
            historyCmd(argv + 1, stdout);
        }
    } else {
        switch (fork()) {
        case -1:
            handleError(NULL, 1);
        case 0:
            if (fds) {
                if (close(fds[0]) < 0) handleError(NULL, 2);  // close unused read end
                if (dup2(fds[1], STDOUT_FILENO) < 0) handleError(NULL, 2);  // overwrite with write end
                if (close(fds[1]) < 0) handleError(NULL, 2);
            }
            if (fd_in >= 0) {
                if (dup2(fd_in, STDIN_FILENO) < 0) handleError(NULL, 2);  // overwrite with preserved read end
                if (close(fd_in) < 0) handleError(NULL, 2);
            }

            if (execvp(argv[0], argv) < 0) handleError(NULL, 2);
        default:
            if (fds) {
                if (close(fds[1]) < 0) handleError(NULL, 1);  // close unused write end
            }
            if (fd_in >= 0) {
                if (close(fd_in) < 0) handleError(NULL, 1);  // close unused preserved read end
            }

            if (wait(NULL) < 0) handleError(NULL, 0);
        }
    }

    return true;
}

void historyCmd(char** args, FILE* f_out) {
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
            handleError(NULL, 0);
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
        fprintf(f_out, "%*d  %s\n", 5, id, history.bufs[idx]);
    }
    fflush(f_out);
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
}

void handleError(char* msg, int should_exit) {
    if (!msg) msg = strerror(errno);
    fprintf(stderr, "error: %s\n", msg);
    if (should_exit) {
        if (bufs_ready) clean();
        if (should_exit == 1) {
            exit(1);
        } else if (should_exit == 2) {
            _exit(1);
        }
    }
}
