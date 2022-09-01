#include "fileSystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[]) {
    char prompt[1024];
    char cmd[1024];
    char arg1[1024];
    char arg2[1024];
    int args;

    if (argc != 2) {
        printf("Veuillez renseigner deux paramètres: %s <NomDuDisque> <NombreDeBlocs>\n", argv[0]  );
        return 1;
    }

    if (!intialisation_disque(argv[1])) {
        printf("Erreur d'initialisation %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    printf("Disque: %s utilisant %d blocks\n", argv[1], disque_size());

    while (1) {
        printf(" SGF-%s> ",argv[0]);
        fflush(stdout);

        if (!fgets(prompt, sizeof(prompt), stdin))
            break;

        if (prompt[0] == '\n')
            continue;
        prompt[strlen(prompt) - 1] = 0;

        args = sscanf(prompt, "%s %s %s", cmd, arg1, arg2);
        if (args == 0)
            continue;

        if (!strcmp(cmd, "format")) {
            if (args == 1) {
                if (fs_format()) {
                    printf("Disque formaté.\n");
                } else {
                    printf("Erreur formatage!\n");
                }
            } else {
                printf("Faites un formatage d'abord\n");
            }
        } else if (!strcmp(cmd, "mount")) {
            if (args == 1) {
                if (fs_mount()) {
                    printf("disque monté.\n");
                } else {
                    printf("montage erreur\n");
                }
            } else {
                printf("Vous devez monter le disque\n");
            }
        } else if (!strcmp(cmd, "help")) {
            printf("Voici les commandes pouvant etre utilisés:\n");
            printf("format\n");
            printf("mount\n");
            printf("help\n");
            printf("exit\n");
            printf("ls\n");
            printf("cd\n");
            printf("touch\n");
            printf("mkdir\n");
            printf("rmdir\n");
            printf("rm\n");
        } else if (!strcmp(cmd, "cd")) {
            if (args == 2) {
                if (fs_cd(arg1)) {
                    printf("changement de répertoire\n");
                }
            }
        } else if (!strcmp(cmd, "touch")) {
            if (args == 2) {
                if (fs_touch(arg1)) {
                    printf("fichier crée\n");
                } else {
                    printf("Erreur création fichier\n");
                }
            }
        } else if (!strcmp(cmd, "ls")) {
            if (args == 1) {
                if (fs_ls()) {
                    printf("informations du répertoire\n");
                } else {
                    printf("Erreur consultation répertoire\n");
                }
            }
        } else if (!strcmp(cmd, "mkdir")) {
            if (args == 2) {
                if (fs_mkdir(arg1)) {
                    printf("répertoire crée\n");
                } else {
                    printf("Erreur création de répertoire\n");
                }
            }
        } else if (!strcmp(cmd, "rmdir")) {
            if (args == 2) {
                if (fs_rmdir(arg1)) {
                    printf("répertoire supprimé\n");
                } else {
                    printf("Erreur suppression du répertoire\n");
                }
            }
        } else if (!strcmp(cmd, "rm")) {
            if (args == 2) {
                if (fs_rm(arg1)) {
                    printf("supprimé\n");
                } else {
                    printf("Erreur suppression\n");
                }
            }
        } else if (!strcmp(cmd, "exit")) {
            break;
        } else {
            printf("commande inconnue: %s\n", cmd);
            // result = 1;
        }
    }

    printf("Fermeture du disque.\n");
    disque_close();

    return 0;
}