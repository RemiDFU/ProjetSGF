#ifndef DISK_H
#define DISK_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#define BLOCK_SIZE 4096

int disk_init(const char *path, int blocks);

int disk_size();

void disk_read(int blocknum, char *data);

void disk_write(int blocknum, const char *data);

void disk_close();


#endif