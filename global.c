MINODE minode[NMINODE];
MTABLE mtable[NMTABLE];
OFT oft[NOFT];
PROC proc[NPROC];
PROC *running;
int ninodes, nblocks, bmap, imap, iblock;
int dev;
char *rootdev = "mydisk"; // default root_device
MINODE *root;



int bit_test(char *buf, int bit)
{
	return buf[bit / 8] & (1 << (bit % 8));
}


int bit_set(char *buf, int bit)
{
	buf[bit / 8] |= 1 << (bit % 8);
	return 0;
}

int bit_clr(char *buf, int bit)
{
	buf[bit / 8] &= ~(1 << (bit % 8));
	return 0;
}

int get_block(int dev, int blk, char *buf)
{
	lseek(dev, blk*BLKSIZE, SEEK_SET);
	int n = read(dev, buf, BLKSIZE);
	if (n<0) printf("get_block [%d %d] error\n", dev, blk);
}

int put_block(int dev, int blk, char *buf)
{
	lseek(dev, blk*BLKSIZE, SEEK_SET);
	int n = write(dev, buf, BLKSIZE);
	if (n != BLKSIZE)
		printf("put_block [%d %d] error\n", dev, blk);
}

int decFreeInodes(int dev, int delta)
{
	char buf[BLKSIZE];
	SUPER *sp;
	GD *gp;

	// Modify super block's free count in dev
	get_block(dev, 1, buf);
	sp = (SUPER*)buf;
	sp->s_free_inodes_count += delta;
	put_block(dev, 1, buf);
	
	// Modify group block's free count in dev
	get_block(dev, 2, buf);
	gp = (GD*)buf;
	gp->bg_free_inodes_count += delta;
	put_block(dev, 2, buf);

	// Modify mountable's cache free count
	mtable[dev].free_inodes += delta;

	return 0;
}

int decFreeBlocks(int dev, int delta)
{
	char buf[BLKSIZE];
	SUPER *sp;
	GD *gp;

	// Modify super block's free count in dev
	get_block(dev, 1, buf);
	sp = (SUPER*)buf;
	sp->s_free_blocks_count += delta;
	put_block(dev, 1, buf);
	
	// Modify group block's free count in dev
	get_block(dev, 2, buf);
	gp = (GD*)buf;
	gp->bg_free_blocks_count += delta;
	put_block(dev, 2, buf);

	// Modify mountable's cache free count
	mtable[dev].free_blocks += delta;

	return 0;
}

MINODE *mialloc() // allocate a FREE minode for use
{
	MINODE *mp;

	for (int i=0; i<NMINODE; i++){
		mp = &minode[i];
		if (mp->refCount == 0){
			mp->refCount = 1;
			return mp;
		}
	}
	printf("FS panic: out of minodes\n");
	return 0;
}

int midalloc(MINODE *mip) // release a used minode
{
	mip->refCount = 0;
}

int ialloc(int dev) // alloc inode from device
{
	char buf[BLKSIZE];

	get_block(dev, mtable[dev].imap, buf);
	for (int bit = 0; bit < mtable[dev].ninodes; bit++){
		if (!bit_test(buf, bit)) { // when bit is not set
			bit_set(buf, bit);
			put_block(dev, mtable[dev].imap, buf); // write imap block back to device
			decFreeInodes(dev, -1);
			return bit+1; // return ino: ino(1~N) --> bit(0~N-1)
		}
	}
	return -1; // No inode is free
}

int idalloc(int dev, int ino)
{
	int bit = ino - 1;
	char buf[BLKSIZE];

	get_block(dev, mtable[dev].imap, buf);
	bit_clr(buf, bit);
	put_block(dev, mtable[dev].imap, buf);
	decFreeInodes(dev, 1); // increment
}

int balloc(int dev) // alloc block from device
{
	char buf[BLKSIZE];

	get_block(dev, mtable[dev].bmap, buf);
	for (int bit = 0; bit < mtable[dev].nblocks; bit++){
		if (!bit_test(buf, bit)) { // when bit is not set
			bit_set(buf, bit);
			put_block(dev, mtable[dev].bmap, buf); // write bmap block back to device
			decFreeBlocks(dev, -1);
			return bit+1; // return bno: bno(1~N) --> bit(0~N-1)
		}
	}
	return -1; // No block is free
}

int bdalloc(int dev, int bno)
{
	int bit = bno - 1;
	char buf[BLKSIZE];

	get_block(dev, mtable[dev].bmap, buf);
	bit_clr(buf, bit);
	put_block(dev, mtable[dev].bmap, buf);
	decFreeBlocks(dev, 1); // increment
}


int minodes_print()
{
	char buf[BLKSIZE];
	unsigned int *bit;
	printf("\n********MINODE**[dev ino refCount]*******\n");
	for (int i = 0; i < 10; i++) //NMINODE
		printf("[%d %d %d]->", minode[i].dev, minode[i].ino, minode[i].refCount);
	printf("\n");
	printf("running->cwd->ino = %d\n", running->cwd->ino);
	printf("inode bitmap\n");
	get_block(dev, mtable[dev].imap, buf);
	bit = (int*)buf;
	for (int i = 0; i < 32; ++i)
		printf("%d", (*bit >> i) % 2);
	printf("\n");
	printf("block bitmap\n");
	get_block(dev, mtable[dev].bmap, buf);
	bit = (int*)buf;
	for (int i = 0; i < 32; ++i)
		printf("%d", (*bit >> i) % 2);
	bit = (int*)buf+1;
	for (int i = 0; i < 32; ++i)
		printf("%d", (*bit >> i) % 2);
	printf("\n");
	printf("*****************************************\n");

}