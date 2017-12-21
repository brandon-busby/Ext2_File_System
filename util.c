MINODE *iget(int dev, int ino)
{
	int i, blk, disp;
	char buf[BLKSIZE];
	MINODE *mip;
	INODE *ip;
	// search for INODE in memory
	for (i = 0; i < NMINODE; i++){
		mip = &minode[i];
		if (mip->refCount && mip->dev == dev && mip->ino == ino){
			mip->refCount++;
			return mip;
		}
	}
	// if here, then inode not found
	// search for empty MINODE
	for (i = 0; i < NMINODE; i++){
		mip = &minode[i];
		if (mip->refCount == 0){
			mip->refCount = 1;
			// assign to (dev, ino)
			mip->dev = dev;
			mip->ino = ino;
			mip->dirty = mip->mounted = 0;
			mip->mountptr = 0;
			// get INODE of ino into buf[]
			blk = (ino - 1) / 8 + iblock;
			disp = (ino - 1) % 8;
			get_block(dev, blk, buf);
			ip = (INODE *)buf + disp;
			// copy INODE tp mip->INODE
			mip->INODE = *ip;
			return mip;
		}
	}
	printf("PANIC: no more free minodes\n");
	return 0;
}

int iput(MINODE *mip)
{
	int blk, disp;
	INODE *ip;
	char buf[BLKSIZE];
	mip->refCount--;
	if (mip->refCount > 0) return 0;
	if (!mip->dirty) return 0;
	// write INODE back to disk
	blk = (mip->ino - 1) / 8 + iblock;
	disp = (mip->ino - 1) % 8;
	get_block(mip->dev, blk, buf);
	ip = (INODE *)buf + disp;
	*ip = mip->INODE;
	put_block(mip->dev, blk, buf);
	return 0;
}

int search(MINODE *mip, char *name)
{
	int i;
	DIR *dp;
	char *cp;
	char buf[BLKSIZE];
	char recName[64];
	INODE *ip = &mip->INODE;
	for(i=0; i<12; i++){
		if (ip->i_block[i] == 0)
			continue;
		get_block(mip->dev, ip->i_block[i], buf);
		dp = (DIR *)(cp = buf);
		while (cp < &buf[BLKSIZE]) {
			if (dp->inode == 0)
				break;
			memcpy(recName, dp->name, dp->name_len);
			recName[dp->name_len] = 0;
			if (strcmp(name, recName) == 0)
				return dp->inode;
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
	}
	return 0;
}

int getino(int *dev, char *pathname)
{
        int i, n, ino, blk, disp;
        char path[64];
        char *name[64];
        INODE *ip;
        MINODE *mip;
        if (strcmp(pathname, "/") == 0)
                return root->ino;
        if (pathname[0] == '/')
                mip = iget(root->dev, root->ino);
        else
                mip = iget(running->cwd->dev, running->cwd->ino);
        // tokenize pathname with name[0 through n-1] and n=numTokens
        strncpy(path, pathname, 64);
        n = (name[0] = strtok(path, "/")) != 0;
        while (n < 64 && (name[n] = strtok(NULL, "/"))) n++;
        // search for ino
        for (i = 0; i < n; i++){
                ino = search(mip, name[i]);
                if (ino == 0){
                        iput(mip);
                        printf("name %s does not exist\n", name[i]);
                        return 0;
                }
                iput(mip);
                mip = iget(*dev, ino);
                *dev = mip->dev;
        }
        iput(mip);
        return ino;
}

int hextoi(char *str)
{
        int num;
        sscanf(str, "%x", &num);
        return num;
}
int octtoi(char *str)
{
        int num;
        sscanf(str, "%o", &num);
        return num;
}
int dectoi(char *str)
{
        int num;
        sscanf(str, "%d", &num);
        return num;
}

int S_ISLNK(int mode) { return 0120000 == (mode & 0120000); }
int S_ISREG(int mode) { return 0100000 == (mode & 0100000); }
int S_ISDIR(int mode) { return 0040000 == (mode & 0040000); }
