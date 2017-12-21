int printDirs(MINODE *mip)
{
	int i;
	INODE *ip;
	DIR *dp;
	char *cp, buf[BLKSIZE], name[64];
	ip = &mip->INODE;
	for (i=0; i<12; i++) {
		if (ip->i_block[i] == 0)
			break;
		get_block(mip->dev, ip->i_block[i], buf);
		cp = buf;
		dp = (DIR *)cp;
		while(cp < &buf[BLKSIZE]) {
			memcpy(name, dp->name, dp->name_len);
			name[dp->name_len] = 0;
			printf("%s: %d\n", name, dp->inode);
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
	}
}

// might mess up if a slash ends a pathname
// e.g. /a/b/c/
// because of strrchr() function
int ls_file(char *pathname)
{
	char t1[] = "xwrxwrxwr";
	char ftime[64];
	char linkname[64];
	int ino, i;
	MINODE *mip;
	INODE *ip;
	strcpy(linkname, pathname);
	if (pathname[0] == '/')
		dev = root->dev;
	else
		dev = running->cwd->dev;
	ino = getino(&dev, pathname);
	if (ino == 0) return -1;
	mip = iget(dev, ino);
	ip = &mip->INODE;
	switch (ip->i_mode & 0xF000) {
		case 0x8000: printf("-"); break;
		case 0x4000: printf("d"); break;
		case 0120000: printf("l"); break;
		default: printf(" ");
	}
	for (i=8; i>=0; i--)
		if (ip->i_mode & (1 << i))
			printf("%c", t1[i]);
		else
			printf("-");
	printf("%4d ", ip->i_links_count);
	printf("%4d ", ip->i_gid);
	printf("%4d ", ip->i_uid);
	printf("%8d ", ip->i_size);
	// print time
	strcpy(ftime, ctime((long *)&ip->i_atime));
	ftime[strlen(ftime) - 1] = 0;
	printf("%s ", ftime);
	// print name
	printf("%s", basename(pathname));
	if (S_ISLNK(ip->i_mode)) {
		readlink(linkname, linkname);
		printf(" --> %s", linkname);
	}
	printf("\n");
	iput(mip);
}

int ls(char *pathname)
{
	int ino, i;
	MINODE *mip;
	char *cp, buf[BLKSIZE], childPath[64];
	if (pathname[0] == '/')
		dev = root->dev;
	else
		dev = running->cwd->dev;
	ino = getino(&dev, pathname);
	if (ino == 0) return -1;
	mip = iget(dev, ino);
	ip = &mip->INODE;
	if (!S_ISDIR(ip->i_mode)) {
		ls_file(pathname);
		return 0;
	}
	for (i=0; i<12; i++) {
		if (ip->i_block[i] == 0)
			continue;
		get_block(mip->dev, ip->i_block[i], buf);
		cp = buf;
		dp = (DIR *)cp;
		while (cp < &buf[BLKSIZE]) {
			if (dp->inode == 0)
				break;
			memset(childPath, 0, 64);
			strcpy(childPath, pathname);
			strcat(childPath, "/");
			strncat(childPath, dp->name, dp->name_len);
			ls_file(childPath);
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
	}
	iput(mip);
	return 0;
}

int cd(char *pathname)
{
	int ino;
	MINODE *mip;
	if (pathname[0] == '/')
		dev = root->dev;
	else
		dev = running->cwd->dev;
	ino = getino(&dev, pathname);
	if (ino == 0) return -1;
	printf("ino = %d\n", ino);
	mip = iget(dev, ino);
	if (!S_ISDIR(mip->INODE.i_mode)) {
		printf("not a valid directory\n");
		return -1;
	}
	iput(running->cwd);
	running->cwd = mip;
	return 0;
}

int pwd(MINODE *curr)
{
	int pino, i;
	MINODE *parent;
	INODE *ip;
	char *cp, buf[BLKSIZE];
	DIR *dp;
	if (curr == root) {
		if (curr == running->cwd)
			printf("/\n");
		return 0;
	}
	pino = search(curr, "..");
	parent = iget(curr->dev, pino);
	pwd(parent);
	ip = &parent->INODE;
	for (i=0; i<12; i++) {
		if (ip->i_block[i] == 0)
			continue;
		get_block(parent->dev, ip->i_block[i], buf);
		cp = buf;
		dp = (DIR *)cp;
		while (cp < &buf[BLKSIZE]) {
			if (dp->inode == curr->ino && strncmp(".", dp->name, dp->name_len)) {
				iput(parent);
				memcpy(buf, dp->name, dp->name_len);
				buf[dp->name_len] = 0;
				printf("/%s", buf);
				if (curr == running->cwd)
					printf("\n");
				return 0;
			}
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
	}
	iput(parent);
	printf("\n");
	return -1;
}
