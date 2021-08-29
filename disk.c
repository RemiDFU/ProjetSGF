#include "disk.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

/* Internal Prototyes */

bool disk_sanity_check(Disk *disk, size_t blocknum, const char *data);

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
Disk *disk_open(const char *path, size_t blocks)
{
    Disk *disk = NULL;
    int FileDescriptor = open(path, "r+");
    if (!FileDescriptor)
        FileDescriptor = open(path, "w+");
    if (!FileDescriptor)
        return 0;

    ftruncate(FileDescriptor, blocks * BLOCK_SIZE);

    disk->blocks = blocks;
    disk->reads = 0;
    disk->writes = 0;

    return disk;
}

int disk_size(Disk *disk)
{
    return disk->blocks;
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
void disk_close(Disk *disk)
{
    if (disk->fd)
    {
        printf("%d disk block reads\n", disk->reads);
        printf("%d disk block writes\n", disk->writes);
        close(disk->fd);
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
ssize_t disk_read(Disk *disk, size_t block, char *data)
{
    if (lseek(disk->fd, block * BLOCK_SIZE, SEEK_SET) < 0)
    {
        return DISK_FAILURE;
    }

    if (read(disk->fd, data, BLOCK_SIZE) != BLOCK_SIZE)
    {
        return DISK_FAILURE;
    }

    disk->reads++;
    return BLOCK_SIZE;
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
ssize_t disk_write(Disk *disk, size_t block, char *data)
{
    if (lseek(disk->fd, block*BLOCK_SIZE, SEEK_SET) < 0) {
    	return DISK_FAILURE;
    }

    if (write(disk->fd, data, BLOCK_SIZE) != BLOCK_SIZE) {
    	return DISK_FAILURE;
    }

    disk->writes++;
    return BLOCK_SIZE;
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
bool disk_sanity_check(Disk *disk, size_t block, const char *data)
{
    return false;
}
