int symlink(char *target, char *linkpath)
{
        int tino, lino;
        MINODE *tmip, *lmip;
        INODE *tip, *lip;
        char linkpathcpy[64], targetcpy[64];
        if (target[0] == '/')
                dev = root->dev;
        else
                dev = running->cwd->dev;
        strcpy(targetcpy, target);
        tino = getino(&dev, target);
        tmip = iget(dev, tino);
        tip = &tmip->INODE;
        if (!S_ISDIR(tip->i_mode) && !S_ISREG(tip->i_mode)) {
                printf("error: target must be a directory or regular file\n");
                iput(tmip);
		return -1;
        }
        strcpy(linkpathcpy, linkpath);
        creat_file(linkpath);
        lino = getino(&dev, linkpathcpy);
        lmip = iget(dev, lino);
        lip = &lmip->INODE;
        lip->i_mode &= ~(0170000);
        lip->i_mode |= 0120000;
        strcpy((char *)lip->i_block, targetcpy);
	lmip->dirty = 1;
        iput(lmip);
	iput(tmip);
}

int readlink(char *pathname, char *linkname)
{
	int ino;
	MINODE *mip;
	INODE *ip;
	if (pathname[0] == '/')
		dev = root->dev;
	else
		dev = running->cwd->dev;
	ino = getino(&dev, pathname);
	mip = iget(dev, ino);
	ip = &mip->INODE;
	if (!S_ISLNK(ip->i_mode)) {
		printf("file must be a symbolic link\n");
		iput(mip);
		return -1;
	}
	strcpy(linkname, (char *)ip->i_block);
	return 0;
}
