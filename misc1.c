#include <time.h>

int access(char *pathname, int mode)
{
	int ino, i, bitmask, i_mode, i_uid, i_gid;
	MINODE *mip;
	char linkname[64];
	if (pathname[0] == '/')
		dev = root->dev;
	else
		dev = running->cwd->dev;
	ino = getino(&dev, pathname);
	if (ino == 0) return -1;
	mip = iget(dev, ino);
	if (S_ISLNK(mip->INODE.i_mode)) {
		readlink(pathname, linkname);
		iput(mip);
		if (linkname[0] == '/')
			dev = root->dev;
		else
			dev = running->cwd->dev;
		ino = getino(&dev, linkname);
		mip = iget(dev, ino);
	}
	i_mode = mip->INODE.i_mode;
	i_uid = mip->INODE.i_uid;
	i_gid = mip->INODE.i_gid;
	iput(mip);
	switch (mode) {
        case 0: bitmask = 0b100; break;
        case 1: bitmask = 0b010; break;
        case 2: bitmask = 0b110; break;
        case 3: bitmask = 0b010; break;
        default: printf("invalid mode\n"); return 0;
        }
        if (running->uid == 0)
                goto hasPermission;
        if (bitmask == (bitmask & i_mode))
                goto hasPermission;
        bitmask <<= 3;
        if (running->gid == i_gid && bitmask == (bitmask & i_mode))
                goto hasPermission;
        bitmask <<= 3;
        if (running->uid == i_uid && bitmask == (bitmask & i_mode))
                goto hasPermission;
	return 0;
	hasPermission:
        // check if already open in an incompatable mode
        for (i=0; i<NFD; i++) {
                if (running->fd[i] == NULL)
                        continue;
                if (running->fd[i]->minodeptr == mip) {
                        if (running->fd[i]->mode || mode) {
                                printf("cannot open %s in an incompatable mode\n", pathname);
                                return 0;
                        }
                }
        }
	return 1;
}

int chmod(char *pathname, int mode)
{
	int ino;
        MINODE *mip;
        char linkname[64];
        if (pathname[0] == '/')
                dev = root->dev;
        else
                dev = running->cwd->dev;
        ino = getino(&dev, pathname);
        if (ino == 0) return -1;
        mip = iget(dev, ino);
        if (S_ISLNK(mip->INODE.i_mode)) {
                readlink(pathname, linkname);
                iput(mip);
                if (linkname[0] == '/')
                        dev = root->dev;
                else
                        dev = running->cwd->dev;
                ino = getino(&dev, linkname);
                mip = iget(dev, ino);
        }
	printf("mode: %o\n", mode);
	mip->INODE.i_mode = mode | (mip->INODE.i_mode & 0777000);
	mip->dirty = 1;
	iput(mip);
}

int chown(char *pathname, int uid, int gid)
{
	int ino;
        MINODE *mip;
        char linkname[64];
        if (pathname[0] == '/')
                dev = root->dev;
        else
                dev = running->cwd->dev;
        ino = getino(&dev, pathname);
        mip = iget(dev, ino);
        if (S_ISLNK(mip->INODE.i_mode)) {
                readlink(pathname, linkname);
                iput(mip);
                if (linkname[0] == '/')
                        dev = root->dev;
                else
                        dev = running->cwd->dev;
                ino = getino(&dev, linkname);
                mip = iget(dev, ino);
        }
        mip->INODE.i_uid = uid;
        mip->INODE.i_gid = gid;
	mip->dirty = 1;
	iput(mip);
	return 0;
}

int touch(char *pathname)
{
	int ino;
	MINODE *mip;
	if (pathname[0] == '/')
		dev = root->dev;
	else
		dev = running->cwd->dev;
	ino = getino(&dev, pathname);
	if (ino == 0) {
		printf("creating file...\n");
		creat_file(pathname);
		return 0;
	}
	mip = iget(dev, ino);
	mip->INODE.i_atime = mip->INODE.i_mtime = time(0L);
	mip->dirty = 1;
	iput(mip);
	return 0;
}
