#ifndef FS_H
#define FS_H

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
int fs_getsize();

// Directories and Files
int fs_touch(char name[]);
int fs_mkdir(char name[]);
int fs_rmdir(char name[]);
int fs_cd(char name[]);
int fs_ls();
int fs_rm(char name[]);
int fs_copyout(char name[], const char *path);
int fs_copyin(const char *path, char name[]);
int fs_dir(char name[]);
void exit();

struct fs_directory rmdir_helper(struct fs_directory parent, char name[]);
struct fs_directory rm_helper(struct fs_directory parent, char name[]);

int do_copyin(const char *filename, int inumber);
int do_copyout(int inumber, const char *filename);

#endif