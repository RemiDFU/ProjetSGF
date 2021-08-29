#include "sfs/disk.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>

#include <unistd.h>

/* Constants */

#define DISK_PATH   "unit_disk.image"
#define DISK_BLOCKS (4)

/* Functions */

void test_cleanup() {
    unlink(DISK_PATH);
}

int test_00_disk_open() {
    //debug("Check bad path");
    Disk *disk = disk_open("/asdf/NOPE", 10);
    assert(disk == NULL);

    //debug("Check bad block size");
    disk = disk_open(DISK_PATH, LONG_MAX);
    assert(disk == NULL);

    //debug("Check disk attributes");
    disk = disk_open(DISK_PATH, 10);
    assert(disk);
    assert(disk->fd      >= 0);
    assert(disk->blocks  == 10);
    assert(disk->reads   == 0);
    assert(disk->writes  == 0);
    disk_close(disk);

    return EXIT_SUCCESS;
}

/* Main execution */

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s NUMBER\n\n", argv[0]);
        fprintf(stderr, "Where NUMBER is right of the following:\n");
        fprintf(stderr, "    0. Test disk_open\n");
        fprintf(stderr, "    1. Test disk_read\n");
        fprintf(stderr, "    2. Test disk_write\n");
        return EXIT_FAILURE;
    }

    int number = atoi(argv[1]);
    int status = EXIT_FAILURE;

    atexit(test_cleanup);

    switch (number) {
        case 0:  status = test_00_disk_open(); break;
        //case 1:  status = test_01_disk_read(); break;
        //case 2:  status = test_02_disk_write(); break;
        default: fprintf(stderr, "Unknown NUMBER: %d\n", number); break;
    }

    return status;
}
