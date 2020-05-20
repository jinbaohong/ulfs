#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
// define shorter TYPES for convenience
typedef struct ext2_group_desc GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode INODE;
typedef struct ext2_dir_entry_2 DIR;
#define BLKSIZE 1024

// Block number of EXT2 FS on FD
#define SUPERBLOCK 1
#define GDBLOCK 2
#define ROOT_INODE 2
// Default dir and regular file modes
#define DIR_MODE 0x41ED
#define FILE_MODE 0x81AE
#define SUPER_MAGIC 0xEF53
#define SUPER_USER 0
// Proc status
#define FREE 0
#define READY 1
#define SLEEP 2
#define BLOCK 3
#define PAUSE 4
#define ZOMBIE 5
// file system table sizes
#define NMINODE 100
#define NMTABLE 10
#define NPROC 2
#define NFD 10
#define NOFT 40

// Open File Table
typedef struct oft{
	int mode;
	int refCount;
	struct minode *minodePtr;
	int offset;
}OFT;

// PROC structure
typedef struct proc{
	struct proc *next;
	int pid;
	int uid;
	int gid;
	int ppid;
	int status;
	struct minode *cwd;
	OFT *fd[NFD];
}PROC;

// In-memory inodes structure
typedef struct minode{
	INODE inode; // disk inode
	int dev, ino;
	int refCount; // use count
	int dirty; // modified flag
	int mounted; // mounted flag
	struct mtable *mntPtr; // mount table pointer
	// int lock; // ignored for simple FS
}MINODE;

// Mount Table structure
typedef struct mtable{
	int dev;
	int ninodes;
	int nblocks;
	int free_blocks;
	int free_inodes;
	int bmap;
	int imap;
	int iblock;
	MINODE *mntDirPtr;
	char devName[64];
	char mntName[64];
}MTABLE;

