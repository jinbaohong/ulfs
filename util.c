



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

int get_inode_from_dev(int dev, int ino, INODE *ip)
{ // ip is inode table base address
	lseek(dev, mtable[dev].iblock*BLKSIZE + (ino-1)*sizeof(INODE), SEEK_SET);
	read(dev, ip, sizeof(INODE));
	return 0;
}

MINODE *get_inode_from_mem(int dev, int ino)
{
	MINODE *mip;
	MTABLE *mp;
	INODE *ip;
	int i, block, offset;
	char buf[BLKSIZE];
	// search in-memory minodes first
	for (i=0; i<NMINODE; i++){
		MINODE *mip = &minode[i];
		if (mip->refCount && (mip->dev==dev) && (mip->ino==ino)){
			mip->refCount++;
			return mip;
		}
	}
	// needed INODE=(dev,ino) not in
	if ((mip = mialloc()) == NULL)  // allocate a FREE minode
		return NULL;
	mip->dev = dev; mip->ino = ino; // assign to (dev, ino)
	get_inode_from_dev(dev, ino, &mip->inode);
	// initialize minode
	mip->refCount = 1; // set to 1 again (first time is in mialloc())
	mip->mounted = 0;
	mip->dirty = 0;
	mip->mntPtr = 0;
	return mip;
}

int put_inode(MINODE *mip)
{
	INODE *ip;
	int i, block, offset;
	char buf[BLKSIZE];

	if (mip == NULL) return 0;
	if (--mip->refCount > 0) return 0;
	if (mip->dirty == 0) return 0;
	// write INODE back to disk
	block = (mip->ino - 1) / 8 + mtable[mip->dev].iblock;
	offset = (mip->ino - 1) % 8;
	// get block containing this inode
	get_block(mip->dev, block, buf);
	ip = (INODE *)buf + offset;
	*ip = mip->inode;
	put_block(mip->dev, block, buf);
	midalloc(mip);
	return 1;
}


int search(MINODE *mip, char *name)
{ // Search dir_ent's inode by name in inode
	char buf[BLKSIZE], temp[256], *cp;
	DIR *dp;
	for (int i = 0; i < 12; i++)
	{ // Assume user only use direct block
		get_block(mip->dev, mip->inode.i_block[i], buf);

		dp = (DIR*)buf;
		cp = buf;

		while(cp < buf + BLKSIZE && dp->rec_len){
		/* The last dir_ent's rec_len will big enough
		 * to fill in data block.
		 */
			strncpy(temp, dp->name, dp->name_len);
			temp[dp->name_len] = 0;
			if (strcmp(temp, name) == 0)
				return dp->inode; // return inode number
			cp += dp->rec_len;
			dp = (DIR *)cp;	
		}
	}
	return -1; // Doesn't find such name in inode inode
}

int getino(int dev, char *pathname)
{
	MINODE *mip;
	int i, ino;
	char *s, tmp_str[256];

	if (!strcmp(pathname, ""))
		return -1;
	if (strcmp(pathname, "/")==0)
		return 2; // return root ino=2
	mip = *pathname == '/' ? get_inode_from_mem(dev, 2) : get_inode_from_mem(dev, running->cwd->ino);

	strcpy(tmp_str, pathname); // strtok() will modify original string.
	s = strtok(tmp_str, "/"); // first call to strtok()
	while(s){ // s is token
		if ((ino = search(mip, s)) == -1){
			put_inode(mip);
			printf("Error: can't find %s\n", s);
			return -1;
		}
		put_inode(mip);
		mip = get_inode_from_mem(dev, ino);
		s = strtok(0, "/"); // call strtok() until it returns NULL
		if (mip->inode.i_mode >> 12 != 4)
			break; // if inode is not dir, then break
	}

	if (s) { // Error: /.../file/dir
		put_inode(mip);
		printf("Error: inode %d is a file, doesn't have dir_ent %s\n", mip->ino, s);
		return -1;
	}
	put_inode(mip);

	return ino;
}

int get_sp(int fd, SUPER *sp)
{
	lseek(fd, BLKSIZE, SEEK_SET);
	read(fd, sp, sizeof(struct ext2_super_block));
	return 0;
}

int check_ext2(int fd)
{
	char buf[BLKSIZE];
	struct ext2_super_block *super;

	super = calloc(sizeof(struct ext2_super_block), 1);

	get_sp(fd, super);
	if (super->s_magic != 0xEF53)
		return -1;
	return 0;
}

int get_myino(MINODE *mip, int *parent_ino)
{
	char buf[BLKSIZE];
	DIR *dir_ent;
	int myino;

	get_block(dev, mip->inode.i_block[0], buf);
	dir_ent = (DIR*)buf; // '.'
	myino = dir_ent->inode;
	dir_ent = (DIR*)(buf + dir_ent->rec_len); // '..'
	*parent_ino = dir_ent->inode; // return parent_ino
	return myino;
}

int get_myname(MINODE *parent_minode, int my_ino, char *my_name)
{
	char buf[BLKSIZE], *cp;
	DIR *dp;
	for (int i = 0; i < 12; i++)
	{ // Assume user only use direct block
		get_block(parent_minode->dev, parent_minode->inode.i_block[i], buf);

		dp = (DIR*)buf;
		cp = buf;

		while(cp < buf + BLKSIZE && dp->rec_len){
			if (my_ino == dp->inode){
				strncpy(my_name, dp->name, dp->name_len);
				my_name[dp->name_len] = '\0';
				return 0;
			}
			cp += dp->rec_len;
			dp = (DIR *)cp;	
		}
	}
	return -1; // Doesn't find such ino in parent_minode
}
