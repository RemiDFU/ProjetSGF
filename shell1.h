#ifndef FS_H
#define FS_H

#include "fs.h"

#include <stdio.h>
#include <string.h>

// core function
int fs_touch(char name[NAMESIZE]);
int fs_mkdir(char name[]);
int fs_rmdir(char name[]);
int fs_cd(char name[]);
int fs_ls();
int fs_rm(char name[]);
int copyout(char name[], const char *path);
int copyin(const char *path, char name[]);
int do_copyin(const char *filename, int inumber);
int do_copyout(int inumber, const char *filename);
int fs_dir(char name[]);
void exit();

// helper function
struct fs_directory fs_add_dir_entry(struct fs_directory dir, int inum, int type, char name[]);
void fs_write_dir_back(struct fs_directory dir);
int fs_dir_lookup(struct fs_directory dir, char name[]);
struct fs_directory fs_read_dir_from_offset(int offset);
struct fs_directory rmdir_helper(struct fs_directory parent, char name[]);
struct fs_directory rm_helper(struct fs_directory parent, char name[]);

#endif
