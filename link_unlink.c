int link(char *oldpath, char *newpath)
{
	int oldino, newino, i;
	char *parent, *child, pathcpy[64];
	MINODE *oldmip, *newmip;
	INODE *oldip, *newip;
	char *cp, buf[BLKSIZE];
	DIR *dp;
	if (oldpath[0] == '/')
		dev = root->dev;
	else
		dev = running->cwd->dev;
	oldino = getino(&dev, oldpath);
	oldmip = iget(dev, oldino);
	oldip = &oldmip->INODE;
	if (!S_ISREG(oldip->i_mode) && !S_ISLNK(oldip->i_mode)) {
		printf("%s is an invalid file type\n", oldpath);
		iput(oldmip);
		return -1;
	}
	strcpy(pathcpy, newpath);
	child = basename(pathcpy);
	parent = dirname(newpath);
	if (parent[0] == '/')
		dev = root->dev;
	else
		dev = running->cwd->dev;
	newino = getino(&dev, parent);
	newmip = iget(dev, newino);
	newip = &newmip->INODE;
	if (!S_ISDIR(newip->i_mode)) {
		printf("%s not directory\n", parent);
		iput(oldmip);
		iput(newmip);
		return -1;
	}
	if (search(newmip, child)) {
		printf("%s/%s already exists", parent, child);
		iput(oldmip);
		iput(newmip);
		return -1;
	}
	if (oldmip->dev != newmip->dev) {
		printf("%s and %s/%s must be on same device\n", oldpath, parent, child);
		iput(oldmip);
		iput(newmip);
		return -1;
	}
	enter_name(newmip, oldino, child);
	oldip->i_links_count++;
	oldmip->dirty = 1;
	iput(oldmip);
	iput(newmip);
}

// change to delete indirect blocks
int truncate(MINODE *mip)
{
	INODE *ip;
	int i;
	int buf1[BLKSIZE / sizeof(int)], buf2[BLKSIZE / sizeof(int)], buf3[BLKSIZE / sizeof(int)];
	int *bp1, *bp2, *bp3;
	ip = &mip->INODE;
	// direct blocks
	for(i=0; i<12; i++) {
		if (ip->i_block[i])
			continue;
		bdealloc(mip->dev, ip->i_block[i]);
	}
	// single indirect blocks
	if (ip->i_block[12]) {
		get_block(mip->dev, ip->i_block[12], (char *)buf1);
		bp1 = buf1;
		while (bp1 < &buf1[BLKSIZE / sizeof(int)]) {
			if (*bp1) bdealloc(mip->dev, *bp1);
			bp1++;
		}
		bdealloc(mip->dev, ip->i_block[12]);
	}
	// double indirect blocks
	if (ip->i_block[13]) {
		get_block(mip->dev, ip->i_block[13], (char *)buf1);
		bp1 = buf1;
		while (bp1 < &buf1[BLKSIZE / sizeof(int)]) {
			if (*bp1) {
				get_block(mip->dev, *bp1, (char *)buf2);
				bp2 = buf2;
				while (bp2 < &buf2[BLKSIZE / sizeof(int)]) {
					if (*bp2) bdealloc(mip->dev, *bp2);
					bp2++;
				}
				bdealloc(mip->dev, *bp1);
			}
			bp1++;
		}
		bdealloc(mip->dev, ip->i_block[13]);
	}
	// triple indirect blocks
	if (ip->i_block[14]) {
                get_block(mip->dev, ip->i_block[14], (char *)buf1);
                bp1 = buf1;
                while (bp1 < &buf1[BLKSIZE / sizeof(int)]) {
                        if (*bp1) {
                                get_block(mip->dev, *bp1, (char *)buf2);
                                bp2 = buf2;
                                while (bp2 < &buf2[BLKSIZE / sizeof(int)]) {
                                        if (*bp2) {
						get_block(mip->dev, *bp2, (char *)buf3);
						bp3 = buf3;
						while (bp3 < &buf3[BLKSIZE / sizeof(int)]) {
							if (*bp3) bdealloc(mip->dev, *bp3);
							bp3++;
						}
						bdealloc(mip->dev, *bp2);
                                        }
					bp2++;
                                }
                                bdealloc(mip->dev, *bp1);
                        }
                        bp1++;
                }
                bdealloc(mip->dev, ip->i_block[14]);
        }
}

int unlink(char *pathname)
{
	int ino, pino;
	MINODE *mip, *pip;
	INODE *ip;
	char *parent, *child;
	child = basename(pathname);
	parent = dirname(pathname);
	if (parent[0] == '/')
		dev = root->dev;
	else
		dev = running->cwd->dev;
	pino = getino(&dev, parent);
	pip = iget(dev, pino);
	if (!S_ISDIR(pip->INODE.i_mode)) {
		printf("%s must be a directory\n", parent);
		return -1;
	}
	
	ino = search(pip, child);
	mip = iget(dev, ino);
	ip = &mip->INODE;
	if (!S_ISREG(ip->i_mode) && !S_ISLNK(ip->i_mode)) {
		printf("%s is an invalid file type\n", pathname);
		return -1;
	}
	ip->i_links_count--;
	if (ip->i_links_count == 0) {
		truncate(mip);
	}
	rm_child(pip, child);
	iput(mip);
	iput(pip);
}
