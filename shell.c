#include "fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[]) {
    char line[1024];
    char cmd[1024];
    char arg1[1024];
    char arg2[1024];
    int inumber, result, args;

    if (argc != 3) {
        printf("use: %s <diskfile> <nblocks>\n", argv[0]);
        return 1;
    }

    if (!disk_init(argv[1], atoi(argv[2]))) {
        printf("couldn't initialize %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    printf("opened emulated disk image %s with %d blocks\n", argv[1], disk_size());

    while (1) {
        printf(" simplefs> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin))
            break;

        if (line[0] == '\n')
            continue;
        line[strlen(line) - 1] = 0;

        args = sscanf(line, "%s %s %s", cmd, arg1, arg2);
        if (args == 0)
            continue;

        if (!strcmp(cmd, "format")) {
            if (args == 1) {
                if (fs_format()) {
                    printf("disk formatted.\n");
                } else {
                    printf("format failed!\n");
                }
            } else {
                printf("use: format\n");
            }
        } else if (!strcmp(cmd, "mount")) {
            if (args == 1) {
                if (fs_mount()) {
                    printf("disk mounted.\n");
                } else {
                    printf("mount failed!\n");
                }
            } else {
                printf("use: mount\n");
            }
        } else if (!strcmp(cmd, "debug")) {
            if (args == 1) {
                fs_debug();
            } else {
                printf("use: debug\n");
            }
        } else if (!strcmp(cmd, "getsize")) {
            if (args == 2) {
                inumber = atoi(arg1);
                result = fs_getsize(inumber);
                if (result >= 0) {
                    printf("inode %d has size %d\n", inumber, result);
                } else {
                    printf("getsize failed!\n");
                }
            } else {
                printf("use: getsize <inumber>\n");
            }
        } else if (!strcmp(cmd, "create")) {
            if (args == 1) {
                inumber = fs_create();
                if (inumber > 0) {
                    printf("created inode %d\n", inumber);
                } else {
                    printf("create failed!\n");
                }
            } else {
                printf("use: create\n");
            }
        } else if (!strcmp(cmd, "delete")) {
            if (args == 2) {
                inumber = atoi(arg1);
                if (fs_delete(inumber)) {
                    printf("inode %d deleted.\n", inumber);
                } else {
                    printf("delete failed!\n");
                }
            } else {
                printf("use: delete <inumber>\n");
            }
        } else if (!strcmp(cmd, "cat")) {
            if (args == 2) {
                if (!copyout(arg1, "/dev/stdout")) {
                    printf("cat failed!\n");
                }
            } else {
                printf("use: cat <inumber>\n");
            }
        } else if (!strcmp(cmd, "copyin")) {
            if (args == 3) {
                if (copyin(arg1, arg2)) {
                    printf("copied file %s to inode %s\n", arg1, arg2);
                } else {
                    printf("copy failed!\n");
                }
            } else {
                printf("use: copyin <path> <filename>\n");
            }
        } else if (!strcmp(cmd, "copyout")) {
            if (args == 3) {
                if (copyout(arg1, arg2)) {
                    printf("copied inode %s to file %s\n", arg1, arg2);
                } else {
                    printf("copy failed!\n");
                }
            } else {
                printf("use: copyout <filename> <path>\n");
            }
        } else if (!strcmp(cmd, "bitmap")) {
            if (args == 1) {
                print_bitmap();
            } else {
                printf("use: bitmap\n");
            }
        } else if (!strcmp(cmd, "touch")) {
            if (args == 2) {
                if (fs_touch(arg1)) {
                    printf("touched\n");
                } else {
                    printf("not touched\n");
                }
            } else {
                printf("use: touch <filename>\n");
            }
        } else if (!strcmp(cmd, "ls")) {
            if (args == 1) {
                if (fs_ls()) {
                    //printf("lsed\n");
                } else {
                    printf("not lsed\n");
                }
            } else {
                printf("use: ls\n");
            }
        } else if (!strcmp(cmd, "mkdir")) {
            if (args == 2) {
                if (fs_mkdir(arg1)) {
                    printf("mkdired\n");
                } else {
                    printf("not mkdired\n");
                }
            } else {
                printf("use: mkdir <dirname>\n");
            }
        } else if (!strcmp(cmd, "rmdir")) {
            if (args == 2) {
                if (fs_rmdir(arg1)) {
                    printf("rmdired\n");
                } else {
                    printf("not rmdired\n");
                }
            } else {
                printf("use: rmdir <dirname>\n");
            }
        } else if (!strcmp(cmd, "cd")) {
            if (args == 2) {
                if (fs_cd(arg1)) {
                    printf("cded\n");
                } else {
                    printf("not cded\n");
                }
            } else {
                printf("use: cd <dirname>\n");
            }
        } else if (!strcmp(cmd, "rm")) {
            if (args == 2) {
                if (fs_rm(arg1)) {
                    printf("rmed\n");
                } else {
                    printf("not rmed\n");
                }
            } else {
                printf("use: rm <filename>\n");
            }
        } else if (!strcmp(cmd, "help")) {
            printf("Commands are:\n");
            printf("    format\n");
            printf("    mount\n");
            printf("    debug\n");
            printf("    bitmap\n");
            printf("    create\n");
            printf("    delete  <inode>\n");
            printf("    cat     <inode>\n");
            printf("    copyin  <file> <inode>\n");
            printf("    copyout <inode> <file>\n");
            printf("    help\n");
            printf("    quit\n");
            printf("    exit\n");
        } else if (!strcmp(cmd, "quit")) {
            break;
        } else if (!strcmp(cmd, "exit")) {
            break;
        } else {
            printf("unknown command: %s\n", cmd);
            printf("type 'help' for a list of commands.\n");
            result = 1;
        }
    }

    printf("closing emulated disk.\n");
    disk_close();

    return 0;
}