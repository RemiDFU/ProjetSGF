#include "fs.h"
#include "disk.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>

#define streq(a, b) (strcmp((a), (b)) == 0)

#define FS_MAGIC 0xf0f03410
#define INODES_PER_BLOCK 128
#define POINTERS_PER_INODE 5
#define POINTERS_PER_BLOCK 1024

#define NAMESIZE 16       // Max Name size for files/directories
#define ENTRIES_PER_DIR 7 // Number of Files/Directory entries within a Directory
#define DIR_PER_BLOCK 8   // Number of Directories per 4KB block
#define BLOCKS 10         // Number of blocks

int *bitmap = NULL;
int bitmap_size;

struct fs_superblock
{
    int magic;
    int nblocks;
    int ninodeblocks;
    int ninodes;
    int ndirblocks;
};

struct fs_dirent
{
    int type;
    int isvalid;
    int inum;
    char name[NAMESIZE];
};

struct fs_directory
{
    int isvalid;
    int inum;
    char name[NAMESIZE];
    struct fs_dirent table[ENTRIES_PER_DIR];
};

struct fs_inode
{
    int isvalid;
    int size;
    int direct[POINTERS_PER_INODE];
    int indirect;
};

union fs_block
{
    struct fs_superblock super;
    struct fs_inode inode[INODES_PER_BLOCK];
    int pointers[POINTERS_PER_BLOCK];
    char data[DISK_BLOCK_SIZE];
    struct fs_directory directories[DIR_PER_BLOCK];
};

// Internal member variables
bool free_block[BLOCKS];
int inode_counter[BLOCKS];
int dir_counter[BLOCKS];
struct fs_directory curr_dir;

int get_new_datablock()
{
    if (bitmap == NULL)
    {
        printf("No disk mounted, please mount first\n");
        return -1;
    }

    union fs_block block;
    disk_read(0, block.data);

    //start at the first datablock instance
    for (int i = block.super.ninodeblocks + 1; i < bitmap_size; i++)
    {
        if (bitmap[i] == 0)
        {
            //zero it out
            memset(&bitmap[i], 0, sizeof(bitmap[0]));
            return i;
        }
    }

    printf("No more room left\n");
    return -1;
}

int fs_format()
{
    union fs_block block;

    if (disk_size() < 3)
    {
        printf("Disk to small, does not meet minimum node size\n");
        return 0;
    }
    else if (bitmap != NULL)
    {
        printf("Cannot format mounted image\n");
        return 0;
    }

    // set the super block
    block.super.magic = FS_MAGIC;
    block.super.nblocks = disk_size();
    block.super.ninodeblocks = disk_size() / 10 + 1;
    block.super.ninodes = 128 * block.super.ninodeblocks;
    block.super.ndirblocks = disk_size() / 100 + 1;

    // write the super block
    disk_write(0, block.data);

    // zero out the inode blocks you just made

    for (int inode_block = (block.super.ninodeblocks) + 1; inode_block < block.super.ninodeblocks - block.super.ndirblocks; inode_block++)
    {
        union fs_block zero;
        memset(zero.data, 0, DISK_BLOCK_SIZE);
        disk_write(inode_block, zero.data);
    }

    for (int i = block.super.nblocks - block.super.ndirblocks; i < block.super.nblocks; i++)
    {
        union fs_block zero;
        struct fs_directory dir;
        dir.inum = -1;
        dir.isvalid = 0;
        memset(dir.table, 0, sizeof(struct fs_dirent) * ENTRIES_PER_DIR);
        for (int j = 0; j < DIR_PER_BLOCK; j++)
        {
            zero.directories[j] = dir;
        }
        disk_write(i, zero.data);
    }

    struct fs_directory root;
    strcpy(root.name, "/");
    root.inum = 0;
    root.isvalid = 1;

    struct fs_dirent temp;
    memset(&temp, 0, sizeof(temp));
    temp.inum = 0;
    temp.type = 0;
    temp.isvalid = 1;
    char tstr1[] = ".";
    char tstr2[] = "..";
    strcpy(temp.name, tstr1);
    memcpy(&(root.table[0]), &temp, sizeof(struct fs_dirent));
    strcpy(temp.name, tstr2);
    memcpy(&(root.table[1]), &temp, sizeof(struct fs_dirent));

    union fs_block dirblock;
    memcpy(&(dirblock.directories[0]), &root, sizeof(root));
    disk_write(block.super.nblocks - 1, dirblock.data);

    return 1;
}

void print_bitmap()
{
    if (bitmap != NULL)
    {
        for (int i = 0; i < bitmap_size; i++)
        {
            printf(" %d", bitmap[i]);
        }
        printf("\n");
    }
    else
    {
        printf("Bitmap NULL\n");
    }
}

void fs_debug()
{
    union fs_block block;

    disk_read(0, block.data);

    printf("superblock:\n");
    if (!(block.super.magic == FS_MAGIC))
    {
        printf("Cannot understand file system\n");
        return;
    }
    printf("    magic number is valid\n");
    printf("    %d blocks\n", block.super.nblocks);
    printf("    %d inode blocks\n", block.super.ninodeblocks);
    printf("    %d inodes\n", block.super.ninodes);

    union fs_block inode_block;
    struct fs_inode inode;
    for (int i = 1; i < block.super.ninodeblocks; i++)
    {

        disk_read(i, inode_block.data);

        for (int i_node = 0; i_node < INODES_PER_BLOCK; i_node++)
        {

            inode = inode_block.inode[i_node];

            if (inode.isvalid)
            {
                printf("inode %d:\n", i_node);
                printf("    size: %d\n", inode.size);
                printf("    direct blocks:");

                for (int d_blocks = 0; d_blocks * 4096 < inode.size && d_blocks < 5; d_blocks++)
                {
                    printf(" %d", inode.direct[d_blocks]);
                }

                printf("\n");

                if (inode.size > 5 * 4096)
                {

                    printf("    indirect: %d\n", inode.indirect);
                    union fs_block temp_block;
                    disk_read(inode.indirect, temp_block.data);

                    printf("    indirect data blocks:");
                    for (int indirect_block = 0; indirect_block < (double)inode.size / 4096 - 5; indirect_block++)
                    {
                        printf(" %d", temp_block.pointers[indirect_block]);
                    }
                    printf("\n");
                }
            }
        }
    }
}

int fs_mount()
{
    union fs_block block;
    disk_read(0, block.data);

    bitmap = calloc(block.super.nblocks, sizeof(int));
    bitmap_size = block.super.nblocks;

    //parse the file system to correctly set the bitmap for the given file system
    union fs_block inode_block;
    struct fs_inode inode;
    for (int i = 1; i < block.super.ninodeblocks; i++)
    {

        disk_read(i, inode_block.data);

        for (int i_node = 0; i_node < INODES_PER_BLOCK; i_node++)
        {

            inode = inode_block.inode[i_node];

            if (inode.isvalid)
            {
                bitmap[i] = 1;
                for (int d_blocks = 0; d_blocks * 4096 < inode.size && d_blocks < 5; d_blocks++)
                {
                    bitmap[inode.direct[d_blocks]] = 1;
                }

                if (inode.size > 5 * 4096)
                {

                    bitmap[inode.indirect] = 1;

                    union fs_block temp_block;
                    disk_read(inode.indirect, temp_block.data);

                    for (int indirect_block = 0; indirect_block < (double)inode.size / 4096 - 5; indirect_block++)
                    {
                        bitmap[temp_block.pointers[indirect_block]] = 1;
                    }
                }
            }
        }
    }

    memset(dir_counter, 0, sizeof(dir_counter));
    union fs_block dirblock;
    for (int dirs = 0; dirs < block.super.ndirblocks; dirs++)
    {
        disk_read(block.super.nblocks - 1 - dirs, dirblock.data);
        for (int offset = 0; offset < DIR_PER_BLOCK; offset++)
        {
            if (dirblock.directories[offset].isvalid == 1)
            {
                dir_counter[dirs]++;
            }
        }
        if (dirs == 0)
        {
            curr_dir = dirblock.directories[0];
        }
    }

    return 1;
}

int fs_create()
{
    if (bitmap == NULL)
    {
        printf("No mounted disk\n");
        return 0;
    }

    union fs_block block;
    disk_read(0, block.data);

    for (int inode_block_index = 1; inode_block_index < block.super.nblocks; inode_block_index++)
    {
        // read the inode block and begin checking it for open spaces
        disk_read(inode_block_index, block.data);

        // must start at one because of the union
        struct fs_inode inode;
        for (int inode_index = 0; inode_index < POINTERS_PER_BLOCK; inode_index++)
        {
            if (inode_index == 0 && inode_block_index == 1)
                inode_index = 1;

            // read space as an inode, and check valid flag
            inode = block.inode[inode_index];

            if (inode.isvalid == 0)
            {

                // if the inode is invalid, we can fill the space safely
                inode.isvalid = true;
                inode.size = 0;
                memset(inode.direct, 0, sizeof(inode.direct));
                inode.indirect = 0;

                bitmap[inode_block_index] = 1;
                block.inode[inode_index] = inode;
                disk_write(inode_block_index, block.data);
                return inode_index + (inode_block_index - 1) * 128;
            }
        }
    }

    printf("Could not create inode, inode blocks are full");
    return 0;
}

int fs_delete(int inumber)
{
    // calculate correct inode block
    int inode_block_index = (inumber + 128 - 1) / 128;

    union fs_block block;
    disk_read(0, block.data);

    if (inode_block_index > block.super.ninodeblocks)
    {
        printf("Inode number outside limit\n");
        return 0;
    }

    // read the inode block and begin checking it for open spaces
    disk_read(inode_block_index, block.data);

    struct fs_inode inode = block.inode[inumber % 128];
    if (inode.isvalid)
    {
        //zero it out and return 1;
        inode = (struct fs_inode){0};
        block.inode[inumber % 128] = inode;
        disk_write(inode_block_index, block.data);
        return 1;
    }
    else
    {
        //cannot delete invalid inode, I think...
        printf("Not a valid inode, cannot delete\n");
        return 0;
    }
}

int fs_getsize(int inumber)
{
    union fs_block block;
    disk_read(0, block.data);

    // calculate correct inode block
    int inode_block_index = (inumber + 128 - 1) / 128;

    if (inode_block_index > block.super.ninodeblocks)
    {
        printf("Inode number outside limit\n");
        return 0;
    }

    disk_read(inode_block_index, block.data);
    struct fs_inode inode = block.inode[inumber % 128];

    if (inode.isvalid)
    {
        return inode.size;
    }

    printf("Invalid inode at inumber\n");
    return -1;
}

int fs_read(int inumber, char *data, int length, int offset)
{
    memset(data, 0, length);
    union fs_block block;
    disk_read(0, block.data);
    if (inumber == 0 || inumber > block.super.ninodes)
    {
        printf("Cannot read; invalid inumber\n");
        return 0;
    }

    // ok I guess were reading now
    int total_bytes_read = 0;
    int inode_block_index = (inumber + 128 - 1) / 128;

    // read the inode block
    disk_read(inode_block_index, block.data);

    // fetch actual inode
    struct fs_inode inode = block.inode[inumber % 128];
    if (!inode.isvalid || inode.size == 0)
    {
        printf("Invalid inode, cannot read\n");
    }
    else
    {
        if (offset >= inode.size)
            return 0;

        // figure out where the **** to start reading
        // parse through each from offset to length or offset up to size, swapping in and out of the blocks necessary
        int direct_index = (int)floor(offset / 4096);
        union fs_block temp_block;

        int upper_limit = length;
        if (inode.size < length + offset)
            upper_limit = inode.size - offset;

        while (direct_index < 5 && total_bytes_read < upper_limit)
        {

            int chunk = 4096;
            if (chunk + total_bytes_read > upper_limit)
                chunk = upper_limit - total_bytes_read;

            disk_read(inode.direct[direct_index], temp_block.data);

            strncat(data, temp_block.data, chunk);
            total_bytes_read += chunk;
            direct_index++;
        }

        if (total_bytes_read < upper_limit)
        {

            //indirect block time
            union fs_block ind_block;
            disk_read(inode.indirect, ind_block.data);

            for (int indirect_block = 0; indirect_block < (double)inode.size / 4096 - 5 && total_bytes_read < upper_limit; indirect_block++)
            {

                disk_read(ind_block.pointers[indirect_block], temp_block.data);

                int chunk = 4096;
                if (chunk + total_bytes_read > upper_limit)
                    chunk = upper_limit - total_bytes_read;
                strncat(data, temp_block.data, chunk);

                total_bytes_read += chunk;
            }
        }
        return total_bytes_read;
    }

    return 0;
}

int fs_write(int inumber, const char *data, int length, int offset)
{
    union fs_block block;
    disk_read(0, block.data);
    if (inumber == 0 || inumber > block.super.ninodes)
    {
        printf("Cannot write; invalid inumber\n");
        return 0;
    }

    // ok I guess were reading now
    int total_bytes_wrote = 0;
    int inode_block_index = (inumber + 128 - 1) / 128;

    // read the inode block
    disk_read(inode_block_index, block.data);

    // fetch actual inode
    struct fs_inode inode = block.inode[inumber % 128];
    if (!inode.isvalid)
    {
        printf("Invalid inode, cannot write\n");
    }
    else
    {

        // figure out where the **** to start writing
        // parse through each from offset to length or offset up to size, swapping in and out of the blocks necessary
        int direct_index = (int)floor(offset / 4096);
        union fs_block temp_block;

        while (direct_index < 5 && total_bytes_wrote < length)
        {
            if (inode.direct[direct_index] == 0)
            {
                int index = get_new_datablock();
                if (index == -1)
                {
                    printf("Not enough space left, cannot write");
                    return -1;
                }
                inode.direct[direct_index] = index;
                bitmap[index] = 1;
            }

            int chunk = 4096;
            if (chunk + total_bytes_wrote > length)
                chunk = length - total_bytes_wrote;

            strncpy(temp_block.data, data, chunk);
            data += chunk;

            disk_write(inode.direct[direct_index], temp_block.data);
            inode.size += chunk;

            total_bytes_wrote += chunk;
            direct_index++;
        }

        if (total_bytes_wrote < length)
        {

            //indirect block time
            union fs_block ind_block;

            if (inode.indirect == 0)
            {
                int index = get_new_datablock();
                if (index == -1)
                {
                    printf("Not enough space left, cannot write");
                    return -1;
                }
                inode.indirect = index;
                bitmap[index] = 1;
            }

            disk_read(inode.indirect, ind_block.data);

            for (int indirect_block = 0; total_bytes_wrote < length; indirect_block++)
            {
                if (ind_block.pointers[indirect_block] == 0)
                {
                    int index = get_new_datablock();
                    if (index == -1)
                    {
                        printf("Not enough space left, cannot write");
                        return -1;
                    }
                    inode.direct[direct_index] = index;
                }

                int chunk = 4096;
                if (chunk + total_bytes_wrote > length)
                    chunk = length - total_bytes_wrote;

                strncpy(temp_block.data, data, chunk);
                data += chunk;

                disk_write(inode.direct[direct_index], temp_block.data);
                inode.size += chunk;

                total_bytes_wrote += chunk;
            }
        }
        block.inode[inumber % 128] = inode;
        disk_write(inode_block_index, block.data);
        return total_bytes_wrote;
    }

    return 0;
}

struct fs_directory fs_add_dir_entry(struct fs_directory dir, int inum, int type, char name[])
{
    struct fs_directory tempdir = dir;

    int idx = 0;
    for (; idx < ENTRIES_PER_DIR; idx++)
    {
        if (tempdir.table[idx].isvalid == 0)
        {
            break;
        }
    }

    if (idx == ENTRIES_PER_DIR)
    {
        printf("Directory entry limit reached..exiting\n");
        tempdir.isvalid = 0;
        return tempdir;
    }

    tempdir.table[idx].inum = inum;
    tempdir.table[idx].type = type;
    tempdir.table[idx].isvalid = 1;
    strcpy(tempdir.table[idx].name, name);

    return tempdir;
}

void fs_write_dir_back(struct fs_directory dir)
{
    int block_idx = (dir.inum / DIR_PER_BLOCK);
    int block_offset = (dir.inum % DIR_PER_BLOCK);

    union fs_block block0, block;
    disk_read(0, block0.data);
    disk_read(block0.super.nblocks - 1 - block_idx, block.data);
    block.directories[block_offset] = dir;

    disk_write(block0.super.nblocks - 1 - block_idx, block.data);
}

int fs_dir_lookup(struct fs_directory dir, char name[])
{
    int offset = 0;
    for (; offset < ENTRIES_PER_DIR; offset++)
    {
        if (
            (dir.table[offset].isvalid == 1) &&
            (streq(dir.table[offset].name, name)))
        {
            break;
        }
    }
    if (offset == ENTRIES_PER_DIR)
    {
        return -1;
    }

    return offset;
}

struct fs_directory fs_read_dir_from_offset(int offset)
{
    if (
        (offset < 0) ||
        (offset >= ENTRIES_PER_DIR) ||
        (curr_dir.table[offset].isvalid == 0) ||
        (curr_dir.table[offset].type != 0))
    {
        struct fs_directory temp;
        temp.isvalid = 0;
        return temp;
    }

    int inum = curr_dir.table[offset].inum;
    int block_idx = (inum / DIR_PER_BLOCK);
    int block_offset = (inum % DIR_PER_BLOCK);

    union fs_block block0, blk;
    disk_read(0, block0.data);
    disk_read(block0.super.nblocks - 1 - block_idx, blk.data);
    return (blk.directories[block_offset]);
}

int fs_touch(char name[NAMESIZE])
{
    if (bitmap == NULL)
    {
        printf("No disk mounted, please mount first\n");
        return -1;
    }

    for (int offset = 0; offset < ENTRIES_PER_DIR; offset++)
    {
        if (curr_dir.table[offset].isvalid)
        {
            if (streq(curr_dir.table[offset].name, name))
            {
                printf("File already exists\n");
                return -1;
            }
        }
    }
    int new_node_idx = fs_create();
    if (new_node_idx == -1)
    {
        printf("Error creating new inode\n");
        return false;
    }

    struct fs_directory temp = fs_add_dir_entry(curr_dir, new_node_idx, 1, name);
    if (temp.isvalid == 0)
    {
        printf("Error adding new file\n");
        return -1;
    }
    curr_dir = temp;

    fs_write_dir_back(curr_dir);

    return 1;
}

int fs_ls()
{
    if (bitmap == NULL)
    {
        printf("No disk mounted, please mount first\n");
        return -1;
    }
    char name[] = ".";
    return fs_dir(name);
}

int fs_dir(char name[])
{
    if (bitmap == NULL)
    {
        printf("No disk mounted, please mount first\n");
        return -1;
    }
    int offset = fs_dir_lookup(curr_dir, name);
    if (offset == -1)
    {
        printf("No such Directory\n");
        return -1;
    }

    struct fs_directory dir = fs_read_dir_from_offset(offset);

    if (dir.isvalid == 0)
    {
        printf("Directory Invalid\n");
        return -1;
    };

    printf("   inum    |       name       | type\n");
    for (int idx = 0; idx < ENTRIES_PER_DIR; idx++)
    {
        struct fs_dirent temp = dir.table[idx];
        if (temp.isvalid == 1)
        {
            if (temp.type == 1)
                printf("%-10u | %-16s | %-5s\n", temp.inum, temp.name, "file");
            else
                printf("%-10u | %-16s | %-5s\n", temp.inum, temp.name, "dir");
        }
    }
    return 1;
}

int fs_mkdir(char name[NAMESIZE])
{
    if (bitmap == NULL)
    {
        printf("No disk mounted, please mount first\n");
        return -1;
    }
    union fs_block zero;
    disk_read(0, zero.data);

    int block_idx = 0;
    for (; block_idx < zero.super.nblocks; block_idx++)
        if (dir_counter[block_idx] < DIR_PER_BLOCK)
            break;

    if (block_idx == zero.super.ndirblocks)
    {
        printf("Directory limit reached\n");
        return -1;
    }

    union fs_block block;
    disk_read(zero.super.nblocks - 1 - block_idx, block.data);

    int offset = 0;
    for (; offset < DIR_PER_BLOCK; offset++)
        if (block.directories[offset].isvalid == 0)
            break;

    if(offset == DIR_PER_BLOCK) {printf("Error in creating directory.\n"); return -1;}

    struct fs_directory new_dir, temp;
    memset(&new_dir, 0, sizeof(struct fs_directory));
    new_dir.inum = block_idx * DIR_PER_BLOCK + offset;
    new_dir.isvalid = 1;
    strcpy(new_dir.name, name);

    char tstr1[] = ".", tstr2[] = "..";
    temp = new_dir;
    temp = fs_add_dir_entry(temp,temp.inum,0,tstr1);
    temp = fs_add_dir_entry(temp,curr_dir.inum,0,tstr2);
    if(temp.isvalid == 0){printf("Error creating new directory\n"); return false;}
    new_dir = temp;

    temp = fs_add_dir_entry(curr_dir,new_dir.inum,0,new_dir.name);
    if(temp.isvalid == 0){printf("Error adding new directory\n"); return false;}
    curr_dir = temp;

    fs_write_dir_back(new_dir);

    fs_write_dir_back(curr_dir);

    dir_counter[block_idx]++;

    return 1;
}

int fs_cd(char name[NAMESIZE]) {
    if (bitmap == NULL)
    {
        printf("No disk mounted, please mount first\n");
        return -1;
    }

    int offset = fs_dir_lookup(curr_dir, name);
    if ((offset == -1) || (curr_dir.table[offset].type == 1)) {
        printf("No such directory\n");
        return -1;
    }

    struct fs_directory temp = fs_read_dir_from_offset(offset);
    if(temp.isvalid == 0) {return -1;}
    curr_dir = temp;
    return 1;
}

struct fs_directory rmdir_helper(struct fs_directory parent, char name[]) {
    struct fs_directory dir, temp;
    int inum, blk_idx, blk_off;
    union fs_block blk, zero;
    disk_read(0, zero.data);

    if (bitmap == NULL)
    {
        printf("No disk mounted, please mount first\n");
        return dir;
    }

    int offset = fs_dir_lookup(parent, name);
    if (offset == -1) {dir.isvalid = 0; return dir;}

    inum = parent.table[offset].inum;
    blk_idx = inum / DIR_PER_BLOCK;
    blk_off = inum % DIR_PER_BLOCK;
    disk_read(zero.super.nblocks - 1 - blk_idx, blk.data);

    dir = blk.directories[blk_off];
    if (dir.isvalid == 0) {return dir;}

    if (streq(dir.name, curr_dir.name)) {printf("Current Directory cannot be removed.\n"); dir.isvalid=0; return dir;}

    for (int ii=0; ii<ENTRIES_PER_DIR; ii++) {
        if ((ii<1)&&(dir.table[ii].isvalid == 1)) {
            temp = rm_helper(dir, dir.table[ii].name);
            if(temp.isvalid == 0) return temp;
            dir = temp;
        }
        dir.table[ii].isvalid = 0;
    }
    disk_read(zero.super.nblocks - 1 - blk_idx, blk.data);

    dir.isvalid = 0;
    blk.directories[blk_off] = dir;
    disk_write(zero.super.nblocks - 1 - blk_idx, blk.data);

    parent.table[offset].isvalid = 0;
    fs_write_dir_back(parent);

    dir_counter[blk_idx]--;

    return parent;
}

struct fs_directory rm_helper(struct fs_directory dir, char name[]) {
    if (bitmap == NULL)
    {
        printf("No disk mounted, please mount first\n");
        dir.isvalid = 0;
        return dir;
    }
    int offset = fs_dir_lookup(dir, name);
    if(offset == -1){printf("No such file/directory\n"); dir.isvalid=0; return dir;}

    if(dir.table[offset].type == 0) {
        return rmdir_helper(dir, name);
    }
    int inum = dir.table[offset].inum;
    printf("%u\n",inum);
    if(!fs_delete(inum)){printf("Failed to remove Inode\n"); dir.isvalid = 0; return dir;}

    dir.table[offset].isvalid = 0;

    fs_write_dir_back(dir);

    return dir;
}

int fs_rmdir(char name[NAMESIZE]) {
    struct fs_directory temp = rmdir_helper(curr_dir, name);
    if (temp.isvalid == 0) {
        return 1;
    }
    return -1;
}