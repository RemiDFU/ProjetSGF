#include "sfs/disk.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

/* Internal Prototyes */

bool    disk_sanity_check(Disk *disk, size_t blocknum, const char *data);
/* External Functions */

/**
 *
 * Opens disk at specified path with the specified number of blocks by doing
 * the following:
 *
 *  1. Allocates Disk structure and sets appropriate attributes.
 *
 *  2. Opens file descriptor to specified path.
 *
 *  3. Truncates file to desired file size (blocks * BLOCK_SIZE).
 *
 * @param       path        Path to disk image to create.
 * @param       blocks      Number of blocks to allocate for disk image.
 *
 * @return      Pointer to newly allocated and configured Disk structure (NULL
 *              on failure).
 **/
Disk *	disk_open(const char *path, size_t blocks) {
    Disk *disk = NULL;
    int fd = fopen(path, "r+");
    if(!fd) fd = fopen(path, "w+");
    if(!fd) return 0;

    ftruncate(fileno(fd), blocks * BLOCK_SIZE);

    disk->blocks = 0;
    disk->reads = 0;
    disk->writes = 0;

    return disk;
}

/**
 * Close disk structure by doing the following:
 *
 *  1. Close disk file descriptor.
 *
 *  2. Report number of disk reads and writes.
 *
 *  3. Releasing disk structure memory.
 *
 * @param       disk        Pointer to Disk structure.
 */
void disk_close(Disk *disk) {
    if(disk->fd) {
        printf("%zu disk block reads\n",disk->reads);
        printf("%zu disk block writes\n",disk->writes);
        fclose(disk->fd);
        disk->fd = 0;
    }
}

/**
 * Read data from disk at specified block into data buffer by doing the
 * following:
 *
 *  1. Performing sanity check.
 *
 *  2. Seeking to specified block.
 *
 *  3. Reading from block to data buffer (must be BLOCK_SIZE).
 *
 * @param       disk        Pointer to Disk structure.
 * @param       block       Block number to perform operation on.
 * @param       data        Data buffer.
 *
 * @return      Number of bytes read.
 *              (BLOCK_SIZE on success, DISK_FAILURE on failure).
 **/
ssize_t disk_read(Disk *disk, size_t block, char *data) {
    disk_sanity_check(disk, block, data);
    fseek(disk->fd , block * BLOCK_SIZE, SEEK_SET);

    if(fread(data, BLOCK_SIZE, 1, disk->fd) == 1) {
        disk->reads++;
        return BLOCK_SIZE;
    } else {
        printf("ERROR: couldn't access simulated disk: %s\n",strerror(errno));
        return DISK_FAILURE;
    }
}

/**
 * Write data to disk at specified block from data buffer by doing the
 * following:
 *
 *  1. Performing sanity check.
 *
 *  2. Seeking to specified block.
 *
 *  3. Writing data buffer (must be BLOCK_SIZE) to disk block.
 *
 * @param       disk        Pointer to Disk structure.
 * @param       block       Block number to perform operation on.
 * @param       data        Data buffer.
 *
 * @return      Number of bytes written.
 *              (BLOCK_SIZE on success, DISK_FAILURE on failure).
 **/
ssize_t disk_write(Disk *disk, size_t block, char *data) {
    disk_sanity_check(disk, block, data);

    fseek(disk->fd,block * BLOCK_SIZE,SEEK_SET);

    if(fwrite(data, BLOCK_SIZE, 1, disk->fd)==1) {
        disk->writes++;
        return BLOCK_SIZE;
    } else {
        printf("ERROR: couldn't access simulated disk: %s\n",strerror(errno));
        return DISK_FAILURE;
    }
}

/* Internal Functions */

/**
 * Perform sanity check before read or write operation:
 *
 *  1. Check for valid disk.
 *
 *  2. Check for valid block.
 *
 *  3. Check for valid data.
 *
 * @param       disk        Pointer to Disk structure.
 * @param       block       Block number to perform operation on.
 * @param       data        Data buffer.
 *
 * @return      Whether or not it is safe to perform a read/write operation
 *              (true for safe, false for unsafe).
 **/
bool disk_sanity_check(Disk *disk, size_t block, const char *data) {
    if(block < 0) {
        printf("ERROR: blocknum (%zu) is negative!\n",block);
        return false;
    }

    if(block >= disk->blocks) {
        printf("ERROR: blocknum (%zu) is too big!\n",block);
        return false;
    }

    if(!data) {
        printf("ERROR: null data pointer!\n");
        return false;
    }
    return true;
}
