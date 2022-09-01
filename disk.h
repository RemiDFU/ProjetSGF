#ifndef DISK_H
#define DISK_H

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BLOCK_SIZE 4096
#define BLOCKS 10         // Nombre de blocs définies

int intialisation_disque(const char *path);

void disque_read(int blocknum, char *data);

void disque_write(int blocknum, const char *data);

void disque_close();

int disque_size();


#endif