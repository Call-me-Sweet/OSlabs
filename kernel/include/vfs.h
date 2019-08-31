#ifndef __VFS_H__
#define __VFS_H__
#include <common.h>
#include <klib.h>
#include <stdint.h>
#define SHELL

//#define L3DEBUG
#define NFILE 16
#define NFILEINFS 80 
//#define l3DEBUG

typedef struct inodeops inodeops_t;
typedef struct superblock superblock_t;
typedef struct filesystem filesystem_t; 
typedef struct fsops fsops_t; 
typedef struct file file_t; 
typedef struct inode inode_t;

typedef struct ext2_dir_entry edt_t;
typedef struct ext2_dir ed_t;

struct Proc{
    inode_t *ino[NFILEINFS];
    char* block[NFILEINFS];
};
// ========================= inode ===========================//
struct inode {
  //...
  uint16_t mod;
  size_t size;
  uint32_t nblocks;//the files only have one node
  off_t offset;  //相对文件头偏移
                 //procfs中表示proc中第几个
  int refcnt;
  void *ptr;       //dev 
  filesystem_t *fs;
  inodeops_t *ops; // 在inode被创建时，由文件系统的实现赋值
                   // inode ops也是文件系统的一部分
  //...
};

struct inodeops {
  int (*open)(file_t *file, int flags);
  int (*close)(file_t *file);
  ssize_t (*read)(file_t *file, char *buf, size_t size);
  ssize_t (*write)(file_t *file, const char *buf, size_t size);
  off_t (*lseek)(file_t *file, off_t offset, int whence);
  int (*mkdir)(const char *name);
  int (*rmdir)(const char *name);
  int (*link)(const char *name, inode_t *inode);
  int (*unlink)(const char *name);
  // 你可以自己设计readdir的功能
};

// ======================= ext2-dir-entry ===========================// 
#define EXT2_FT_REG_FILE 0x1
#define EXT2_FT_DIR 0x10

struct ext2_dir_entry{
    uint32_t ino;//the number of the inode of this file
    uint32_t ftype;
    uint32_t valid; // 1 means already used
    char name[32];
};

struct ext2_dir{
    struct ext2_dir_entry etrs[NFILEINFS];
    uint32_t maxuse;//if it's i then the maxused is i-1
};
// ========================= filesystem ==============================//
#define RAMBLOCK (2<<18)
#define MAXFS 7
#define INOSIZE (sizeof(struct inode)*NFILEINFS) 
#define FILEBLOCK (2<<11) //4KB
struct filesystem {
  
  const char *name;
  off_t fsoff;
  off_t inooff[NFILEINFS];//the offset in filesystem,相对文件系统头偏移
  off_t blooff;//the first block ,相对文件系统头偏移
  //uint8_t valid; //0 means already be mnt otherwise is 1;

  fsops_t *ops;
  device_t *dev;
};

struct fsops {
  void (*init)(struct filesystem *fs, const char *name, device_t *dev);
  inode_t *(*lookup)(struct filesystem *fs, const char *path, int flags);
  int (*close)(inode_t *inode);
};


struct file {
  inode_t *inode;
  uint64_t offset;
  char name[128];
  //...
};

// =============================== vfs ===============================//
//------ lseek -------//
#define MNTNUM 5
#define LSEEK_CUR 0x1
#define LSEEK_SET 0x10
#define LSEEK_END 0x100
//------ access ------//
#define F_OK 0x1
#define DIR_OK 0x10 
//------ open ------//
#define O_CREAT 0x1
#define O_RDONLY 0x10
#define O_WRONLY 0x100
#define O_RDWR 0x110
#define O_DIR 0x1000
//#define O_APPEND 0x10000
//#define O_TRUNC 0x100000
//------ mkdir & access------//
#define  O_CHECK 0x10000 //inoder to not print warning "there no such file or dir"
#define O_RM 0x10000000//the longest len of int

typedef struct{
   char name[50];
   filesystem_t *fs;
}mpt_t;
/*
typedef struct{

}
*/


typedef struct {
  void (*init)();
  int (*access)(const char *path, int mode);
  int (*mount)(const char *path, filesystem_t *fs);
  int (*unmount)(const char *path);
  int (*mkdir)(const char *path);
  int (*rmdir)(const char *path);
  int (*link)(const char *oldpath, const char *newpath);
  int (*unlink)(const char *path);
  int (*open)(const char *path, int flags);
  ssize_t (*read)(int fd, void *buf, size_t nbyte);
  ssize_t (*write)(int fd, void *buf, size_t nbyte);
  off_t (*lseek)(int fd, off_t offset, int whence);
  int (*close)(int fd);
} MODULE(vfs);

#endif
