#include <ext2fs/ext2_fs.h>

#pragma once
#ifndef TYPE_H
#define TYPE_H

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER *sp;
GD *gp;
INODE *ip;
DIR *dp;

#define BLKSIZE     1024
#define NMINODE      100
#define NFD           16
#define NPROC          4
#define NMT	      16

typedef struct minode {
  INODE INODE;
  int dev, ino;
  int refCount;
  int dirty;
  int mounted;
  struct mntable *mountptr;
}MINODE;

struct mntable {
	int dev;
	MINODE *minodeptr;
	char devname[64];
	int ninodes;
	int nblocks;
	int imap;
	int bmap;
	int iblock;
};

typedef struct oft {
  int  mode;
  int  refCount;
  MINODE *minodeptr;
  int  offset;
}OFT;

typedef struct proc {
  struct proc *next;
  int          pid;
  int          uid;
  int          gid;
  MINODE      *cwd;
  OFT         *fd[NFD];
}PROC;

#endif

