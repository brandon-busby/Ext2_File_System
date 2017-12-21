#include <time.h>

int open_file(char *filename, int mode)
{
	int ino, i;
	MINODE *mip;
	INODE *ip;
	OFT *oftp;
	if (filename[0] == '/')
		dev = root->dev;
	else
		dev = running->cwd->dev;
	ino = getino(&dev, filename);
	mip = iget(dev, ino);
	ip = &mip->INODE;
	if (!S_ISREG(ip->i_mode)) {
		printf("%s must be regular file\n", filename);
		return -1;
	}
	if (!access(filename, mode)) {
		iput(mip);
		return -1;
	}
	// allocate free openFileTable (OFT)
	oftp = malloc(sizeof(OFT));
	oftp->mode = mode;
	oftp->refCount = 1;
	oftp->minodeptr = mip;
	switch (mode) {
	case 0: oftp->offset = 0; break;
	case 1: truncate(mip);
		oftp->offset = 0; break;
	case 2: oftp->offset = 0; break;
	case 3: oftp->offset = mip->INODE.i_size; // APPEND mode
		break;
	}
	for (i=0; i<NFD; i++) {
		if (running->fd[i] == NULL)
			break;
	}
	if (running->fd[i] != NULL) {
		printf("too many open instances of %s\n", filename);
	}
	running->fd[i] = oftp;
	mip->INODE.i_atime = mode ? mip->INODE.i_mtime = time(0L) : time(0L);
	mip->dirty = 1;
	return i;
}

int close_file(int fd)
{
	OFT *oftp;
	if (fd < 0 || fd >= NFD) {
		printf("file descriptor not in range\n");
	}
	if (running->fd[fd] == NULL) {
		printf("no such OFT entry\n");
		return -1;
	}
	oftp = running->fd[fd];
	running->fd[fd] = 0;
	oftp->refCount--;
	if (oftp->refCount > 0)
		return 0;
	// last user of this OFT entry ==> dispose of the MINODE[]
	iput(oftp->minodeptr);
	return 0;
}

// watch out for positioning beyond file boundries
int lseek_file(int fd, int position)
{
	OFT *oftp;
	int originalPosition;
        if (fd < 0 || fd >= NFD) {
                printf("file descriptor not in range\n");
		return -1;
        }
        if (running->fd[fd] == NULL) {
                printf("no such OFT entry\n");
                return -1;
        }
        oftp = running->fd[fd];
	originalPosition = oftp->offset;
	if (position < 0)
		oftp->offset = 0;
	else if (position >= oftp->minodeptr->INODE.i_size)
		oftp->offset = oftp->minodeptr->INODE.i_size - 1;
	else
		oftp->offset = position;
	return originalPosition;
}

int pfd()
{
	int i;
	OFT *oftp;
	printf("fd\tmode\toffset\tINODE\n");
	printf("--\t----\t------\t-----\n");
	for (i=0; i<NFD; i++) {
		if ((oftp = running->fd[i]) == NULL)
			continue;
		printf("%d\t", i);
		switch (oftp->mode) {
		case 0: printf("READ\t"); break;
		case 1: printf("WRITE\t"); break;
		case 2: printf("R/W\t"); break;
		case 3: printf("APPEND\t");
		}
		printf("%d\t", oftp->offset);
		printf("[%d,%d]\n", oftp->minodeptr->dev, oftp->minodeptr->ino);
	}
}

int dup(int fd)
{
	OFT *oftp;
        int i;
        if (fd < 0 || fd >= NFD) {
                printf("file descriptor not in range\n");
        }
        if (running->fd[fd] == NULL) {
                printf("no such OFT entry\n");
                return -1;
        }
        oftp = running->fd[fd];
	for (i=0; i<NFD; i++)
		if (running->fd[i] == NULL) {
			running->fd[i] = oftp;
			oftp->refCount++;
			return i;
		}
	printf("too many open instances of file\n");
	return -1;	
}

int dup2(int fd, int gd)
{
	OFT *oftp;
        int i;
        if (fd < 0 || fd >= NFD) {
                printf("file descriptor not in range\n");
        }
        if (running->fd[fd] == NULL) {
                printf("no such OFT entry\n");
                return -1;
        }
        oftp = running->fd[fd];
	if (gd < 0 || gd >= NFD) {
		printf("FD copy not in range\n");
		return -1;
	}
	if (running->fd[gd] != NULL)
		close_file(gd);
	running->fd[gd] = oftp;
	oftp->refCount++;
	return 0;
}
