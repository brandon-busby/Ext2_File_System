int read_file(int fd, char *buf, int nbytes)
{
	OFT *oftp;
	MINODE *mip;
	INODE *ip;
	int blk, lbk, start, remain, avail, count, toread;
	char *cq, *cp, readBuf[BLKSIZE];
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
        // if mode not RD or RW
        if (oftp->mode != 0 && oftp->mode != 2) {
                printf("file not open for read mode\n");
                return -1;
        }
	oftp = running->fd[fd];
	mip = oftp->minodeptr;
	ip = &mip->INODE;
	avail = ip->i_size - oftp->offset; // num bytes still available in file
	cq = buf; // cq points at buf
	count = 0;
	while (nbytes > 0 && avail > 0) {
		// get block number
		lbk = oftp->offset / BLKSIZE;
		start = oftp->offset % BLKSIZE;
		if (lbk < 12) { // lbk is a direct block
			blk = ip->i_block[lbk];
		} else if (lbk >= 12 && lbk < 256 + 12) { // indirect blocks
			get_block(mip->dev, ip->i_block[12], (char *)blkBuf);
			blk = blkBuf[lbk - 12];
		} else { // double indirect blocks
			lbk -= 256 + 12;
			get_block(mip->dev, ip->i_block[13], (char *)blkBuf);
			get_block(mip->dev, blkBuf[lbk / 256], (char *)blkBuf);
			blk = blkBuf[lbk % 256];
		}
		// get the data block into buffer
		get_block(mip->dev, blk, readBuf);
		// copy from start to buf, at mode remain bytes in this block
		cp = readBuf + start;
		remain = BLKSIZE - start;
		// to read is minimum of remain, avail, and nbytes
		toread = (nbytes < remain ? nbytes : remain);
		toread = (toread < avail ? toread : avail);
		// copy from readBuf into buf
		memcpy(&buf[count], cp, toread);
		*cp += toread;
		oftp->offset += toread;
		count += toread;
		avail -= toread;
		nbytes -= toread;
		remain -= toread;
		// if one data block is not enough, loop back to OUTER while for more...
	}
	return count; // count is the actual number of bytes read
}

int cat(char *filename)
{
	char buf[BLKSIZE];
	int n;
	int fd = open_file(filename, 0);
	while (n = read_file(fd, buf, BLKSIZE)) {
		buf[n] = 0;
		printf("%s", buf);
	}
	close_file(fd);
}
