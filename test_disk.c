#include "disk.h"
#include <stdio.h>

/* Constants */

#define DISK_PATH   "unit_disk.image"
#define DISK_BLOCKS (4)

/* Functions */

int test_00_disk_open() {
    Disk *disk = disk_open("/asdf/NOPE", 10);
    if (disk == NULL) {
        printf("disk == NULL");
    }

    return EXIT_SUCCESS;
}

int main(int argc, char const *argv[])
{
    int status = EXIT_FAILURE;

    status = test_00_disk_open();

    return status;
}
