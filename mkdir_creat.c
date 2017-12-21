int enter_name(MINODE *pip, int myino, char *myname)
{
	int i, need_len, ideal_len, remain, name_len;
	INODE *ip;
	char *cp, buf[BLKSIZE];
	DIR *dp;
	ip = &pip->INODE;
	name_len = strlen(myname);
	need_len = 4*((8 + name_len + 3)/4);
	for (i=0; i<12; i++) {
		if (ip->i_block[i] == 0)
			break;
		get_block(pip->dev, ip->i_block[i], buf);
		cp = buf;
		dp = (DIR *)cp;
		while(cp + dp->rec_len < &buf[BLKSIZE]) {
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
		ideal_len = 4*((8 + dp->name_len + 3)/4);
		remain = dp->rec_len - ideal_len;
		if (remain >= need_len) {
			dp->rec_len = ideal_len;
			cp += dp->rec_len;
			dp = (DIR *)cp;
			dp->inode = myino;
			dp->rec_len = remain;
			dp->name_len = name_len;
			memcpy(dp->name, myname, name_len);
			put_block(pip->dev, ip->i_block[i], buf);
			return 0;
		}
	}
	if (i >= 12) {
		printf("directory full\n");
		return -1;
	}
	ip->i_block[i] = balloc(pip->dev);
	ip->i_blocks += 2;
	get_block(pip->dev, ip->i_block[i], buf);
	cp = buf;
	dp = (DIR *)cp;
	dp->inode = myino;
	dp->rec_len = BLKSIZE;
	dp->name_len = name_len;
	memcpy(dp->name, myname, name_len);
	put_block(pip->dev, ip->i_block[i], buf);
	return 0;
}

int mymkdir(MINODE *pip, char *name)
{
        MINODE *mip;
        INODE *ip;
        int ino, bno;
        char *cp, buf[BLKSIZE];
        DIR *dp;
        ino = ialloc(pip->dev);
	printf("ino = %d\n", ino);
        bno = balloc(pip->dev);
        mip = iget(pip->dev, ino);
        ip = &mip->INODE;
        ip->i_mode = 0x41ED;
        ip->i_uid = running->uid;
        ip->i_gid = running->gid;
        ip->i_size = BLKSIZE;
        ip->i_links_count = 2;
        ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
        ip->i_blocks = 2;
        ip->i_block[0] = bno;
	memset(&ip->i_block[1], 0, sizeof(ip->i_block[0])*15);
        mip->dirty = 1;
        iput(mip);
	get_block(pip->dev, bno, buf);
        cp = buf;
        dp = (DIR *)cp;
        dp->inode = ino;
        dp->rec_len = 12;
        dp->name_len = 1;
        memcpy(dp->name, ".", 1);
        cp += dp->rec_len;
        dp = (DIR *)cp;
        dp->inode = pip->ino;
        dp->rec_len = 1012;
        dp->name_len = 2;
        memcpy(dp->name, "..", 2);
        put_block(pip->dev, bno, buf);
        enter_name(pip, ino, name);
	return 0;
}

int make_dir(char *pathname)
{
	MINODE *mip, *pip;
	char pathcpy[64];
	char *parent, *child;
	int pino;
	if (pathname[0] == '/') {
		mip = root;
		dev = root->dev;
	} else {
		mip = running->cwd;
		dev = mip->dev;
	}
	strcpy(pathcpy, pathname);
	child = basename(pathcpy);
	parent = dirname(pathname);
	pino = getino(&dev, parent);
	pip = iget(dev, pino);
	if (!S_ISDIR(pip->INODE.i_mode)) {
		printf("%s is not a directory\n", parent);
		iput(pip);
		return -1;
	}
	if (search(pip, child)) {
		printf("child already exists\n");
		iput(pip);
		return -1;
	}
	mymkdir(pip, child);
	pip->INODE.i_links_count++;
	pip->INODE.i_atime = time(0L);
	pip->dirty = 1;
	iput(pip);
	return 0;
}

int mycreat(MINODE *pip, char *name)
{
	MINODE *mip;
        INODE *ip;
        int ino;
        ino = ialloc(pip->dev);
        printf("ino = %d\n", ino);
        mip = iget(pip->dev, ino);
        ip = &mip->INODE;
        ip->i_mode = 0x81A4;
        ip->i_uid = running->uid;
        ip->i_gid = running->gid;
        ip->i_size = 0;
        ip->i_links_count = 1;
        ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
        ip->i_blocks = 0;
        memset(&ip->i_block[0], 0, sizeof(ip->i_block[0])*16);
        mip->dirty = 1;
        iput(mip);
        enter_name(pip, ino, name);
	return 0;
}

int creat_file(char *pathname)
{
	MINODE *mip, *pip;
	char *parent, *child;
	int pino;
	if (pathname[0] == '/') {
		mip = root;
		dev = root->dev;
	} else {
		mip = running->cwd;
		dev = mip->dev;
	}
	child = basename(pathname);
	parent = dirname(pathname);
	pino = getino(&dev, parent);
	pip = iget(dev, pino);
	if (!S_ISDIR(pip->INODE.i_mode)) {
		printf("%s is not a directory\n", parent);
		iput(pip);
		return -1;
	}
	if (search(pip, child)) {
		printf("child already exists\n");
		iput(pip);
		return -1;
	}
	mycreat(pip, child);
	pip->INODE.i_atime = time(0L);
	pip->dirty = 1;
	iput(pip);
	return 0;
}
