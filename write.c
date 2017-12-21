int write_file(int fd, char *buf, int nbytes)
{
	OFT *oftp;
        MINODE *mip;
        INODE *ip;
        int blk, lbk, ptrblk, start, remain, count, towrite;
        char *cq, *cp, writeBuf[BLKSIZE];
	int blkBuf[BLKSIZE / sizeof(int)];
        if (fd < 0 || fd >= NFD) {
                printf("file descriptor out of range\n");
                return -1;
        }
        oftp = running->fd[fd];
        if (oftp == NULL) {
                printf("no such file desctiptor\n");
                return -1;
        }
        // if mode not WR or RW or APPEND
        if (oftp->mode != 1 && oftp->mode != 2 && oftp->mode != 3) {
                printf("file not open for a modify mode\n");
                return -1;
        }
        oftp = running->fd[fd];
	mip = oftp->minodeptr;
	ip = &mip->INODE;
	cq = buf; // cq points at buf[]
	count = 0;
	while (nbytes > 0) {
		lbk = oftp->offset / BLKSIZE;
		start = oftp->offset % BLKSIZE;
		if (lbk < 12) { // direct blocks
			if (ip->i_block[lbk] == 0) { // if no data yet
				ip->i_block[lbk] = balloc(mip->dev); // must allocate a block
				memset(writeBuf, 0, BLKSIZE);
				put_block(mip->dev, ip->i_block[lbk], writeBuf);
			}
			blk = ip->i_block[blk]; // blk should be a disk block now
		} else if (lbk >= 12 && lbk < 256 + 12) { // single indirect blocks
			if (ip->i_block[12] == 0)
				ip->i_block[12] = balloc(mip->dev);
			get_block(mip->dev, ip->i_block[12], (char *)blkBuf);
			if (blkBuf[lbk - 12] == 0) {
				blkBuf[lbk - 12] = balloc(mip->dev);
				put_block(mip->dev, ip->i_block[12], (char *)blkBuf);
			}
			blk = blkBuf[lbk - 12];
		} else { // double indirect blocks
			lbk -= 256 + 12;
			if (ip->i_block[13] == 0)
				ip->i_block[13] = balloc(mip->dev);
			get_block(mip->dev, ip->i_block[13], (char *)blkBuf);
			if (blkBuf[lbk / 256] == 0) {
				blkBuf[lbk / 256] = balloc(mip->dev);
				put_block(mip->dev, ip->i_block[13], (char *)blkBuf);
			}
			ptrblk = blkBuf[lbk / 256];
			get_block(mip->dev, ptrblk, (char *)blkBuf);
			if (blkBuf[lbk % 256] == 0) {
				blkBuf[lbk %256] = balloc(mip->dev);
				put_block(mip->dev, ptrblk, (char *)blkBuf);
			}
			blk = blkBuf[lbk % 256];
		}
		// all cases come to here: write to the data block
		get_block(mip->dev, blk, writeBuf); // read disk block onto writeBuf[]
		cp = writeBuf + start; // cp points at startByte in writeBuf[]
		remain = BLKSIZE - start; // number of bytes that remain in this block
		towrite = nbytes < remain ? nbytes : remain;
		memcpy(&writeBuf[count], cq, towrite);
		cq += towrite;
		nbytes -= towrite;
		remain -= towrite;
		count += towrite;
		oftp->offset += towrite;
		if (oftp->offset > ip->i_size) // especially for RW|APPEND mode
			ip->i_size = oftp->offset;
		put_block(mip->dev, blk, writeBuf); // write writeBuf[] to disk
		// loop back to while to write more ... until nbytes are written
	}
	mip->dirty = 1; // mark mip dirty for iput()
	printf("wrote %d char into file descriptor fd=%d\n", count, fd);
	return count;
}

int cp(char *src, char *dest)
{
	int fd, gd, n;
	char buf[BLKSIZE];
	fd = open_file(src, 0); // open for RD
	touch(dest);
	gd = open_file(dest, 2); // open for RW
	while (n = read_file(fd, buf, BLKSIZE))
		write_file(gd, buf, n); // notice the n in write_file()
	close_file(fd);
	close_file(gd);
	return 0;
}

int mv(char *src, char *dest)
{
	int sdev, ddev;
	if (src[0] == '/')
		sdev = root->dev;
	else
		sdev = running->cwd->dev;
	if (getino(&sdev, src) == 0) {
		printf("%s must exist\n", src);
		return -1;
	}
	if (dest[0] == '/')
		ddev = root->dev;
	else
		ddev = running->cwd->dev;
	getino(&ddev, dest);
	if(sdev == ddev) // same device
		link(src, dest);
	else // otherwise different devices
		cp(src, dest);
	unlink(src);
	return 0;
}
