#ifndef FS_H
#define FS_H

#include "disk.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>

// Constants

#define streq(a, b) (strcmp((a), (b)) == 0)

#define FS_MAGIC 0xf0f03410
#define INODES_PER_BLOCK 128
#define POINTERS_PER_INODE 5
#define POINTERS_PER_BLOCK 1024

#define NAMESIZE 16       // Max Name size for files/directories
#define ENTRIES_PER_DIR 7 // Number of Files/Directory entries within a Directory
#define DIR_PER_BLOCK 8   // Number of Directories per 4KB block
#define BLOCKS 10         // Number of blocks

// Struct

struct fs_superblock {
    int magic;
    int nblocks;
    int ninodeblocks;
    int ninodes;
    int ndirblocks;
};

struct fs_dirent {
    int type;
    int isvalid;
    int inum;
    char name[NAMESIZE];
};

struct fs_directory {
    int isvalid;
    int inum;
    char name[NAMESIZE];
    struct fs_dirent table[ENTRIES_PER_DIR];
};

struct fs_inode {
    int isvalid;
    int size;
    int direct[POINTERS_PER_INODE];
    int indirect;
};

union fs_block {
    struct fs_superblock super;
    struct fs_inode inode[INODES_PER_BLOCK];
    int pointers[POINTERS_PER_BLOCK];
    char data[BLOCK_SIZE];
    struct fs_directory directories[DIR_PER_BLOCK];
};

// Internal member variables
int free_block[BLOCKS];
int inode_counter[BLOCKS];
int dir_counter[BLOCKS];
struct fs_directory curr_dir;

// core function
int fs_create();

int fs_delete(int inumber);

int fs_read(int inumber, char *data, int length, int offset);

int fs_write(int inumber, const char *data, int length, int offset);

// Helper functions
void fs_debug();

void print_bitmap();

int fs_format();

int fs_mount();

int fs_getsize(int inumber);

int get_new_datablock();

// Directories and Files
int fs_touch(char name[NAMESIZE]);

int fs_mkdir(char name[]);

int fs_rmdir(char name[]);

int fs_cd(char name[]);

int fs_ls();

int fs_rm(char name[]);

int copyout(char name[], const char *path);

int copyin(const char *path, char name[]);

int fs_dir(char name[]);

struct fs_directory fs_add_dir_entry(struct fs_directory dir, int inum, int type, char name[]);

void fs_write_dir_back(struct fs_directory dir);

int fs_dir_lookup(struct fs_directory dir, char name[]);

struct fs_directory fs_read_dir_from_offset(int offset);

struct fs_directory rmdir_helper(struct fs_directory parent, char name[]);

struct fs_directory rm_helper(struct fs_directory parent, char name[]);

#endif