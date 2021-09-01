#include "disk.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

/* Constants */

#define DISK_PATH   "unit_disk.image"
#define DISK_BLOCKS (4)

int main(int argc, char const *argv[])
{

    Disk *disk = NULL;
    disk = malloc(10 * sizeof(int));

    disk = disk_open(DISK_PATH, 10);
    assert(disk);
    assert(disk->fd      >= 0);
    assert(disk->blocks  == 10);
    assert(disk->reads   == 0);
    assert(disk->writes  == 0);
    disk_close(disk);

    printf("hello\n");

    return EXIT_SUCCESS;
}
