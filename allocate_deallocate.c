int get_block(int dev, int blk, char buf[])
{
    lseek(dev, (long)blk*BLKSIZE, 0);
    read(dev, buf, BLKSIZE);
}

int put_block(int dev, int blk, char buf[])
{
    lseek(dev, (long)blk*BLKSIZE, 0);
    write(dev, buf, BLKSIZE);
}

int tst_bit(char *buf, int bit)
{
	int i, j;
	i = bit / 8;
	j = bit % 8;
	if (buf[i] & (1 << j))
		return 1;
	return 0;
}

int set_bit(char *buf, int bit)
{
	int i, j;
	i = bit / 8;
	j = bit % 8;
	buf[i] |= (1 << j);
}

int clr_bit(char *buf, int bit)
{
	int i, j;
	i = bit / 8;
	j = bit % 8;
	buf[i] &= ~(1 << j);
}

int decFreeInodes(int dev)
{
	SUPER *sp;
	GD *gp;
	char buf[BLKSIZE];
	// dec free INODEs count in SUPER
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	sp->s_free_inodes_count--;
	put_block(dev, 1, buf);
	// dec free INODEs count in GD
	get_block(dev, 2, buf);
	gp = (GD *)buf;
	gp->bg_free_inodes_count--;
	put_block(dev, 2, buf);
	return 0;
}

int incFreeInodes(int dev)
{
	SUPER *sp;
	GD *gp;
	char buf[BLKSIZE];
	// inc free INODEs count in SUPER
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	sp->s_free_inodes_count++;
	put_block(dev, 1, buf);
	// inc free INODEs count in GD
	get_block(dev, 2, buf);
	gp = (GD *)buf;
	gp->bg_free_inodes_count++;
	put_block(dev, 2, buf);
	return 0;
}

int decFreeBlocks(int dev)
{
	SUPER *sp;
	GD *gp;
	char buf[BLKSIZE];
	// dec free blocks count in SUPER
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	sp->s_free_blocks_count--;
	put_block(dev, 1, buf);
	// dev free blocks count in GD
	get_block(dev, 2, buf);
	gp = (GD *)buf;
	gp->bg_free_blocks_count--;
	put_block(dev, 2, buf);
	return 0;
}

int incFreeBlocks(int dev)
{
	SUPER *sp;
	GD *gp;
	char buf[BLKSIZE];
	// inc free blocks count in SUPER
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	sp->s_free_blocks_count++;
	put_block(dev, 1, buf);
	// inc free blocks count in GD
	get_block(dev, 2, buf);
	gp = (GD *)buf;
	gp->bg_free_blocks_count++;
	put_block(dev, 2, buf);
	return 0;
}

int ialloc(int dev)
{
	int i;
	char buf[BLKSIZE];
	// read inode_bitmap block
	get_block(dev, imap, buf);
	for (i=0; i<ninodes; i++)
		if (tst_bit(buf, i) == 0){
			set_bit(buf, i);
			decFreeInodes(dev);
			put_block(dev, imap, buf);
			return (i + 1);
		}
	printf("no more free inodes\n");
	return 0;
}

int balloc(int dev)
{
	int i;
	char buf[BLKSIZE];
	// reade block_bitmap block
	get_block(dev, bmap, buf);
	for (i=0; i<nblocks; i++)
		if (tst_bit(buf, i) == 0) {
			set_bit(buf, i);
			decFreeBlocks(dev);
			put_block(dev, bmap, buf);
			return i;
		}
	printf("no more free blocks\n");
	return -1;
}

int idealloc(int dev, int ino)
{
	char buf[BLKSIZE];
	get_block(dev, imap, buf);
	clr_bit(buf, ino-1);
	incFreeInodes(dev);
	put_block(dev, imap, buf);
	return 0;
}

int bdealloc(int dev, int bno)
{
	char buf[BLKSIZE];
	get_block(dev, bmap, buf);
	clr_bit(buf, bno);
	incFreeBlocks(dev);
	put_block(dev, bmap, buf);
	return 0;
}
