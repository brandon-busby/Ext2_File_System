int rm_child(MINODE *pip, char *name)
{
	int i;
	char *cp, *cp2, buf[BLKSIZE], entName[64];
	DIR *dp, *dp2;
	INODE *ip;
	ip = &pip->INODE;
	// search parent INODE data blocks for entry of name
	for (i=0; i<12; i++) {
		if (ip->i_block[i] == 0)
			continue;
		get_block(pip->dev, ip->i_block[i], buf);
		cp = buf;
		dp = (DIR *)cp;
		while (cp < &buf[BLKSIZE]) {
			memcpy(entName, dp->name, dp->name_len);
			entName[dp->name_len] = 0;
			if (strcmp(entName, name) == 0) {
				goto FoundChild;
			}
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
	}
	printf("error: could not find %s in parent directory\n", name);
	return -1;
	FoundChild:
	// if last entry in block
	if (cp + dp->rec_len == &buf[BLKSIZE]) {
		// if only entry in data block
		if (cp == buf) {
			// deallocate block
			bdealloc(pip->dev, ip->i_block[i]);
			// shift upper blocks down
			memmove(&ip->i_block[i], &ip->i_block[i+1], 11-i);
			ip->i_block[11] = 0;
			return 0;
		}
		// else, search for previous dirent and increase its rec_len
		cp2 = buf;
		dp2 = (DIR *)cp2;
		while (cp2 + dp2->rec_len != cp) {
			cp2 += dp2->rec_len;
			dp2 = (DIR *)cp2;
		}
		dp2->rec_len += dp->rec_len;
	} else {
		// else, shift later blocks down
		cp2 = cp;
		dp2 = (DIR *)cp;
		// copy contents of next directory into current directory
		while (cp2 + dp2->rec_len < &buf[BLKSIZE]) {
			cp2 += dp2->rec_len;
			dp2 = (DIR *)cp2;
			dp->inode = dp2->inode;
			dp->name_len = dp2->name_len;
			memcpy(dp->name, dp2->name, dp2->name_len);
			// if next directory is last directory
			// then add its rec_len to curr rec_len
			dp->rec_len += (cp2 + dp2->rec_len < &buf[BLKSIZE] ? 0 : dp2->rec_len);
			dp = dp2; // no need to increase cp
		}
	}
	put_block(pip->dev, ip->i_block[i], buf);
	return 0;
}

int remove_dir(char *pathname)
{
	int ino, pino, i, n;
	MINODE *mip, *pip;
	INODE *ip;
	char *cp, buf[BLKSIZE];
	DIR *dp;
	// get inode into memory
	if (pathname[0] == '/')
		dev = root->dev;
	else
		dev = running->cwd->dev;
	ino = getino(&dev, pathname);
	mip = iget(dev, ino);
	ip = &mip->INODE;
	// check ownership of inode (if not super user or owner)
	if (running->uid != 0 || running->uid != ip->i_uid) {
		printf("cannot remove: user %d not owner or su\n", running->uid);
		iput(mip);
		return -1;
	}
	// check is directory
	if (!S_ISDIR(ip->i_mode)) {
		printf("cannot remove: not a directory\n");
		iput(mip);
		return -1;
	}
	// check not busy
	if (mip->refCount > 1) {
		printf("cannot remove: currently in use\n");
		iput(mip);
		return -1;
	}
	// check no sub-directories
	if (ip->i_links_count >2) {
		printf("cannot remove: not empty\n");
		iput(mip);
		return -1;
	}
	// check empty
	n = 0;
	for (i=0; i<12; i++) {
		if (ip->i_block[i] == 0)
			continue;
		get_block(mip->dev, ip->i_block[i], buf);
		cp = buf;
		dp = (DIR *)cp;
		while (cp < &buf[BLKSIZE]) {
			n++;
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
	}
	if (n > 2) {
		printf("cannot remove: not empty\n");
		iput(mip);
		return -1;
	}
	// get parent DIR's ino and MINODE
	pino = search(mip, "..");
	pip = iget(mip->dev, pino);
	// Deallocate block and inode of directory
	for (i=0; i< 12; i++) {
		if (ip->i_block[i] == 0)
			continue;
		bdealloc(mip->dev, ip->i_block[i]);
	}
	idealloc(mip->dev, mip->ino);
	iput(mip);
	// remove entry from parent DIR
	rm_child(pip, basename(pathname));
	pip->INODE.i_links_count--;
	pip->INODE.i_atime = time(0L);
	pip->dirty = 1;
	iput(pip);
	return 0;
}
