#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <ext2fs/ext2_fs.h>
#include <sys/types.h>
#include <time.h>

#include "type.h"

// global variables

MINODE minode[NMINODE];		// global minode[] array
MINODE *root;			// root pointer: the /
PROC proc[NPROC], *running;	// PROC; using only proc[0]
struct mntable mountTable[NMT]; // mount table

int fd, dev;					// file descriptor or dev
int nblocks, ninodes, bmap, imap, iblock;	// FS constants

#include "allocate_deallocate.c"
#include "util.c"
#include "mount_umount.c"
#include "cd_pwd.c"
#include "mkdir_creat.c"
#include "rmdir.c"
#include "link_unlink.c"
#include "symlink_readlink.c"
#include "misc1.c"
#include "open_close_lseek.c"
#include "read.c"
#include "write.c"

char *disk = "mydisk";
char *selfDir = ".";
char line[128], *cmd, *param[64];
char buf[BLKSIZE];

// function prototypes
int init();
int quit();

int main(int argc, char *argv[])
{
	SUPER *sp;
	GD *gp;
	int i;
	// get device
	if (argc > 1)
		disk = argv[1];
	if ((dev = open(disk, O_RDWR)) < 0){
		printf("open %s failed\n", disk);
		exit(1);
	}
	// check FS is an EXT2 FS
	printf("checking EXT2 FS: ");
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	if (sp->s_magic != 0xEF53){
		printf("FAILED\n");
		exit(1);
	}
	printf("PASSED\n");
	// get global constants
	nblocks = sp->s_blocks_count;
	ninodes = sp->s_inodes_count;
	get_block(dev, 2, buf);
	gp = (GD *)buf;
	imap = gp->bg_inode_bitmap;
	bmap = gp->bg_block_bitmap;
	iblock = gp->bg_inode_table;
	// FS initialization
	init();
	mount_root();
	printf("creating P0 as running process\n");
	running = &proc[0];
	running->cwd = &minode[0];
	running->cwd->refCount++;
	// command processing loop
	while (1){
		printf("input command [ls|cd|pwd|quit|mkdir|creat|rmdir|link|unlink"
			"|symlink|access|chmod|chown|touch|open|close|lseek|pfd|dup"
			"|dup2|cat|cp|mv]: ");
		line[strcspn(fgets(line, 128, stdin), "\r\n")] = 0;
		cmd = strtok(line, " ");
		if (cmd == NULL) {
			cmd = line;
			strcpy(line, " ");
		}
		i = 0;
		while (param[i] = strtok(NULL, " ")) i++;
		if (param[0] == NULL)
			param[i] = selfDir;
		// execute the cmd
		if (strcmp(cmd, "ls") == 0)
			ls(param[0]);
		if (strcmp(cmd, "cd") == 0)
			cd(param[0]);
		if (strcmp(cmd, "pwd") == 0)
			pwd(running->cwd);
		if (strcmp(cmd, "quit") == 0)
			quit();
		if (strcmp(cmd, "mkdir") == 0)
			make_dir(param[0]);
		if (strcmp(cmd, "creat") == 0)
			creat_file(param[0]);
		if (strcmp(cmd, "rmdir") == 0)
			remove_dir(param[0]);
		if (strcmp(cmd, "link") == 0)
			link(param[0], param[1]);
		if (strcmp(cmd, "rm") == 0 || strcmp(cmd, "unlink") == 0)
			unlink(param[0]);
		if (strcmp(cmd, "symlink") == 0)
			symlink(param[0], param[1]);
		if (strcmp(cmd, "access") == 0 && param[1])
			printf("%s\n", access(param[0], dectoi(param[1])) ? "YES" : "NO");
		if (strcmp(cmd, "chmod") == 0)
			chmod(param[1], octtoi(param[0]));
		if (strcmp(cmd, "chown") == 0 && param[1] && param[2])
			chown(param[0], dectoi(param[1]), dectoi(param[2]));
		if (strcmp(cmd, "touch") == 0 && param[0])
			touch(param[0]);
		if (strcmp(cmd, "open") == 0 && param[1])
			printf("FD = %d\n", open_file(param[0], dectoi(param[1])));
		if (strcmp(cmd, "close") == 0)
			close_file(dectoi(param[0]));
		if (strcmp(cmd, "lseek") == 0 && param[1])
			printf("originalPosition = %d\n", lseek_file(dectoi(param[0]), dectoi(param[1])));
		if (strcmp(cmd, "pfd") == 0)
			pfd();
		if (strcmp(cmd, "dup") == 0)
			printf("FD = %d\n", dup(dectoi(param[0])));
		if (strcmp(cmd, "dup2") == 0 && param[1])
			dup2(dectoi(param[0]), dectoi(param[1]));
		if (strcmp(cmd, "cat") == 0)
			cat(param[0]);
		if (strcmp(cmd, "cp") == 0)
			cp(param[0], param[1]);
		if (strcmp(cmd, "mv") == 0)
			mv(param[0], param[1]);
	}
	return 0;
}

int init()
{
        int i;
        // clear MINODES
        for (i = 0; i < NMINODE; i++)
                minode[i].refCount = 0;
        // initialize process 0
        proc[0].pid = 1;
        proc[0].uid = 0;
        proc[0].cwd = 0;
        memset(proc[0].fd, 0, sizeof(OFT)*NFD);
        // initialize process 1
        proc[1].pid = 2;
        proc[1].uid = 1;
        proc[1].cwd = 0;
        memset(proc[1].fd, 0, sizeof(OFT)*NFD);
        return 0;
}

int quit()
{
	int i;
	for (i=0; i < NMINODE; i++) {
		if (minode[i].refCount != 0)
			iput(&minode[i]);
	}
	exit(0);
}

