int mount_root()
{
	printf("mounting root...\n");
	root = iget(dev, 2);
}

int mount(char *filesys, char *mntpt)
{
	int i, ino;
	MINODE *mip;
	INODE *ip;
	char filesyscpy[46], *devname, buf[BLKSIZE];
	strcpy(filesyscpy, filesys);
	devname = basename(filesyscpy);
	if (filesys == NULL && mntpt == NULL) {
		printf("Mounted FS:\n");
		// display current mounted FS
		return 0;
	}
	for (i=0; i<NMT; i++) {
		if (mountTable[i].dev) continue;
		if (strcmp(devname, mountTable[i].devname) == 0) {
			printf("FS already mounted\n");
			return -1;
		}
	}
	for (i=0; i<NMT; i++)
		if (mountTable[i].dev == 0)
			break;
	if (mountTable[i].dev) {
		printf("too many mounted devices\n");
		return -1;
	}
	dev = open_file(filesys, 2); // open FS for RW
	// check FS is an EXT2 FS
        printf("checking EXT2 FS: ");
        get_block(fd, 1, buf);
        sp = (SUPER *)buf;
        if (sp->s_magic != 0xEF53){
                printf("FAILED\n");
                return -1;
        }
        printf("PASSED\n");
	ino = getino(&dev, filesys);
	mip = iget(dev, ino);
	ip = &mip->INODE;
	if (!S_ISDIR(ip->i_mode)) {
		printf("mount point must be directory");
		iput(mip);
		return -1;
	}
	if (mip->refCount > 1) {
		printf("mount point is busy\n");
		iput(mip);
		return -1;
	}
	mountTable[i].dev = dev;
	strcpy(mountTable[i].devname, devname);
	mountTable[i].nblocks = sp->s_blocks_count;
	mountTable[i].ninodes = sp->s_inodes_count;
	get_block(dev, 2, buf);
	gp = (GD *)buf;
	mountTable[i].imap = gp->bg_inode_bitmap;
	mountTable[i].bmap = gp->bg_block_bitmap;
	mountTable[i].iblock = gp->bg_inode_table;
	mountTable[i].minodeptr = mip;
	mip->mounted = 1;
	mip->mountptr = &mountTable[i];
	return 0;
}

int umount(char *filesys)
{
	int i, j;
	char filesyscpy[64], *devname;
	MINODE *mip;
	strcpy(filesyscpy, filesys);
	devname = basename(filesyscpy);
	for (i=0; i<NMT; i++) {
		if (mountTable[i].dev == 0)
			continue;
		if (strcmp(filesys, mountTable[i].devname) == 0)
			goto FoundDev;
	}
	printf("mounted device not found\n");
	return -1;
	FoundDev:
	for (j=0; j<NMINODE; j++)
		if (minode[j].dev == mountTable[i].dev && minode[j].refCount > 0) {
			if (&minode[j] == mountTable[i].minodeptr && minode[j].refCount > 1) {
				printf("mounted device busy\n");
				return -1;
			}
		} // above checks that all inodes on device are not busy AND mount point is not busy
	mountTable[i].minodeptr->mounted = 0;
	iput(mountTable[i].minodeptr);
	mountTable[i].dev = 0;
	return 0;
}
