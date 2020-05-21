
int jquit() // write all modified minodes to disk
{
	int i;
	for (i=0; i<NMINODE; i++){
		MINODE *mip = &minode[i];
		if (mip->refCount && mip->dirty){
			mip->refCount = 0;
			put_inode(mip);
		}
	}
	exit(0);
}


int inode_print(INODE *inode, int inode_no, char *name)
{
	#define EXT2_S_IFSOCK	0xC000
	#define EXT2_S_IFLNK	0xA000
	#define EXT2_S_IFREG	0x8000
	#define EXT2_S_IFBLK	0x6000
	#define EXT2_S_IFDIR	0x4000
	#define EXT2_S_IFCHR	0x2000
	#define EXT2_S_IFIFO	0x1000
	char mode_string[11];
	memset(mode_string, '-', 10);
	mode_string[10] = '\0';

	switch (inode->i_mode & 0xF000) {
		case EXT2_S_IFSOCK:
		case EXT2_S_IFLNK:
		case EXT2_S_IFREG:
			break;
		case EXT2_S_IFDIR:
			mode_string[0] = 'd';
			break;
		default:
			break;
	}
	if (inode->i_mode & 0x0001) mode_string[9] = 'x';
	if (inode->i_mode & 0x0002) mode_string[8] = 'w';
	if (inode->i_mode & 0x0004) mode_string[7] = 'r';
	if (inode->i_mode & 0x0008) mode_string[6] = 'x';
	if (inode->i_mode & 0x0010) mode_string[5] = 'w';
	if (inode->i_mode & 0x0020) mode_string[4] = 'r';
	if (inode->i_mode & 0x0040) mode_string[3] = 'x';
	if (inode->i_mode & 0x0080) mode_string[2] = 'w';
	if (inode->i_mode & 0x0100) mode_string[1] = 'r';

	printf("%d %s %d ", inode_no, mode_string, inode->i_links_count);
	printf("%s ", getpwuid(inode->i_uid)->pw_name);
	printf("%s ", getgrgid(inode->i_gid)->gr_name);
	printf("%4d ", inode->i_size);

	char time_string[1000];
	time_t i_mtime = inode->i_mtime;
	struct tm *ppp = localtime(&i_mtime);
	strftime(time_string, 1000, " %b %d %H:%M", ppp);
	printf("%s %s\n", time_string, name);

}


int dir_print(int dev, INODE *inode_dir)
{ // Search dir_ent's inode by name in inode
	char buf[BLKSIZE], temp[256], *cp;
	DIR *dp;
	MINODE *mip;

	for (int i = 0; i < 12; i++)
	{ // Assume user only use direct block
		get_block(dev, inode_dir->i_block[i], buf);

		dp = (DIR*)buf;
		cp = buf;
		while(cp < buf + BLKSIZE && dp->rec_len){
			strncpy(temp, dp->name, dp->name_len);
			temp[dp->name_len] = '\0';
			mip = get_inode_from_mem(dev, dp->inode);
			inode_print(&mip->inode, dp->inode, temp);
			put_inode(mip);
			cp += dp->rec_len;
			dp = (DIR *)cp;	
		}
	}
}

int jls(char *pathname)
{
	int ino;
	MINODE *mip;

	if (!strcmp(pathname, "")) // "ls " means "ls ."
		pathname = ".";
	if ((ino = getino(dev, pathname)) == -1)
		return -1;
	mip = get_inode_from_mem(dev, ino);
	if (mip->inode.i_mode >> 12 == 4) // when inode is DIR
		dir_print(dev, &mip->inode);
	else // when inode is FILE
		inode_print(&mip->inode, ino, pathname);

	put_inode(mip);
	return 0;
}

int jchdir(char *pathname)
{
	int ino;
	MINODE *mip;

	if (!strcmp(pathname, "")) // "chdir " means "chdir /"
		pathname = "/";
	if ((ino = getino(dev, pathname)) == -1)
		return -1;
	mip = get_inode_from_mem(dev, ino);
	if (mip->inode.i_mode >> 12 != 4) {// when inode is FILE
		printf("Error: %s: is not a directory\n", pathname);
		return -1;
	}
	put_inode(running->cwd);
	running->cwd = mip;
	return 0;
}

int _jpwd(struct minode *mip)
{
	int my_ino, parent_ino;
	char my_name[256];
	MINODE *parent_mip;

	my_ino = get_myino(mip, &parent_ino);
	parent_mip = get_inode_from_mem(dev, parent_ino);
	if (my_ino != parent_ino) // Not root, go recursion
		_jpwd(parent_mip);
	else{
		printf("/");
		return 0;
	}
	get_myname(parent_mip, my_ino, my_name);
	put_inode(parent_mip);
	printf("%s/", my_name);
}

int jpwd(struct minode *cwd)
{
	printf("Running pwd\n");
	_jpwd(cwd);
	printf("\n");
}


int ideal_len(int name_len)
{
	return 4 * ((11 + name_len) / 4);
}

int enter_name(int dev, MINODE *mip_d, int ino_b, char *bname)
{ // Enter (ino_b, bname) into mip_d
	int need_d,	last_ideal, remain, i;
	char buf_d[BLKSIZE], *cp;
	DIR *dir_d;

	need_d = ideal_len(strlen(bname)); // base's dir_ent demand bytes
	i = 0;
	while (i < 12){
		if (!mip_d->inode.i_block[i]) {// dataBlock i is NULL
			mip_d->inode.i_block[i] = balloc(dev); // alloc a block for it
			mip_d->dirty = 1;
			mip_d->inode.i_size += BLKSIZE; // dir's size + BLKSIZE
			get_block(dev, mip_d->inode.i_block[i], buf_d);
			dir_d = (DIR*)buf_d;
			dir_d->inode = ino_b;
			dir_d->rec_len = BLKSIZE;
			dir_d->name_len = strlen(bname);
			dir_d->file_type = 2; // ????????????????
			strcpy(dir_d->name, bname);
			dir_d->name[strlen(bname)] = '\0';
			break;
		}
		get_block(dev, mip_d->inode.i_block[i], buf_d);
		dir_d = (DIR*)buf_d;
		cp = buf_d;
//?????????????????????
		while(cp  + dir_d->rec_len < buf_d + BLKSIZE){
			cp += dir_d->rec_len;
			dir_d = (DIR *)cp;	
		}
		last_ideal = ideal_len(dir_d->name_len);
		if (dir_d->rec_len - last_ideal < need_d){
			i++; // when remain < need_d ---> this block's remain is not enough
			continue;
		}
		// Now, remain is sufficient to put base's dir in
		dir_d->rec_len = last_ideal; // modify last dir_ent's rec_len
		// Find next dir_ent starting point
		cp += dir_d->rec_len;
		dir_d = (DIR *)cp;
		// Set newest dir_ent's content
		dir_d->inode = ino_b;
		dir_d->rec_len = buf_d + BLKSIZE - cp;
		dir_d->name_len = strlen(bname);
		dir_d->file_type = 2; //?????????????????
		strcpy(dir_d->name, bname);
		dir_d->name[strlen(bname)] = '\0';
		break;
	}
	// if (i >= 12)

	put_block(dev, mip_d->inode.i_block[i], buf_d); // Write dir's dataBlock back
	return 0;
}

int jmkdir(char *pathname)
{
	char dname[256], bname[256], buf_d[BLKSIZE], buf_b[BLKSIZE], *cp;
	int ino_d, ino_b; // dir's ino, base's ino
	int bno_d, bno_b;
	MINODE *mip_d, *mip_b;
	DIR *dir_d, *dir_b;

	// Make sure dir exist, so we can make new base
	_dbname(pathname, dname, bname);

	if ((ino_d = getino(dev, dname)) == -1){
		printf("Error: %s doesn't exist\n", dname);
		return -1;
	}

	// Get dir minode
	if (!(mip_d = get_inode_from_mem(dev, ino_d)))
		return -1;

	// allocate inode for base
	if ((ino_b = ialloc(dev)) == -1){
		printf("Error: ialloc for ino_b\n");
		return -1;
	}

	// Set base's inode meta data
	if (!(mip_b = get_inode_from_mem(dev, ino_b)))
		return -1;
	mip_b->inode.i_mode = 0x41ED; // 040755: DIR type and permissions
	mip_b->inode.i_uid = running->uid; // owner uid
	mip_b->inode.i_gid = running->gid; // group Id
	mip_b->inode.i_size = BLKSIZE; // size in bytes
	mip_b->inode.i_links_count = 2; // parent referece me and i reference myself too
	mip_b->inode.i_atime = mip_b->inode.i_ctime = mip_b->inode.i_mtime = time(0L);
	mip_b->inode.i_blocks = 2; // LINUX: Blocks count in 512-byte chunks
	mip_b->inode.i_block[0] = bno_b = balloc(dev); // new DIR has one data block
	for (int i = 1; i < 15; i++)
		mip_b->inode.i_block[i] = 0;
	mip_b->dirty = 1; // mark minode dirty
	put_inode(mip_b); // write INODE to disk

	// Set base's data block: "." and ".." entry
	memset(buf_b, 0, BLKSIZE);
	get_block(dev, bno_b, buf_b);
	dir_b = (DIR*)buf_b;
	/* make . entry: 4 + 2 + 1(file_type) + 1 + 1 = 9 bytes,
	 * but rec_len should aligned on 4 bytes boundary -> 12 bytes.
	 */
	dir_b->inode = ino_b; // 4
	dir_b->rec_len = 12;  // 2
	dir_b->name_len = 1;  // 1
	dir_b->name[0] = '.'; // 1
	// make .. entry: pino=parent DIR ino, blk=allocated block
	dir_b = (DIR*)((char *)dir_b + 12);
	dir_b->inode = ino_d;
	dir_b->rec_len = BLKSIZE-12; // rec_len spans block
	dir_b->name_len = 2;
	dir_b->name[0] = dir_b->name[1] = '.';
	put_block(dev, bno_b, buf_b); // write to blk on diks


	/* Set dir's data block */
	enter_name(dev, mip_d, ino_b, bname);

	/* Set dir's inode meta data */
	mip_d->inode.i_links_count += 1;
	mip_d->dirty = 1;
	put_inode(mip_d);

}


int jcreat(char *pathname)
{
	char dname[256], bname[256], buf_d[BLKSIZE], buf_b[BLKSIZE], *cp;
	int ino_d, ino_b; // dir's ino, base's ino
	int bno_d, bno_b;
	MINODE *mip_d, *mip_b;
	DIR *dir_d, *dir_b;

	// Make sure dir exist, so we can make new base
	_dbname(pathname, dname, bname);

	if ((ino_d = getino(dev, dname)) == -1){
		printf("Error: %s doesn't exist\n", dname);
		return -1;
	}

	// Get dir minode
	if (!(mip_d = get_inode_from_mem(dev, ino_d)))
		return -1;

	// allocate inode for base
	if ((ino_b = ialloc(dev)) == -1){
		printf("Error: ialloc for ino_b\n");
		return -1;
	}

	// Set base's inode meta data
	if (!(mip_b = get_inode_from_mem(dev, ino_b)))
		return -1;
	mip_b->inode.i_mode = 0x81A4; // REG file type, permission bits set to 0644
	mip_b->inode.i_uid = running->uid;
	mip_b->inode.i_gid = running->gid;
	mip_b->inode.i_size = 0; // file's initial size is 0
	mip_b->inode.i_links_count = 1; // only parent referece me
	mip_b->inode.i_atime = mip_b->inode.i_ctime = mip_b->inode.i_mtime = time(0L);
	mip_b->inode.i_blocks = 0; // LINUX: Blocks count in 512-byte chunks
	for (int i = 0; i < 15; i++)
		mip_b->inode.i_block[i] = 0;
	mip_b->dirty = 1; // mark minode dirty
	put_inode(mip_b); // write INODE to disk

	/* Set dir's data block */
	enter_name(dev, mip_d, ino_b, bname);

	/* Set dir's inode meta data */
	put_inode(mip_d);

}

int isDirEmpty(MINODE *mip)
{ // return 1 for empty, 0 for not empty
	char buf[BLKSIZE], *cp;
	DIR *dir;
	int cnt;

	get_block(dev, mip->inode.i_block[0], buf);
	dir = (DIR*)buf;
	cp = buf;
	cnt = 0;
	while(cp < buf + BLKSIZE && dir->rec_len){
		cp += dir->rec_len;
		dir = (DIR *)cp;
		cnt++;
	}
	return cnt > 2 ? 0 : 1;
}

int compact_i_block(MINODE *mip)
{
	;
}

int rm_child(MINODE *mip, char *name)
{
	char buf[BLKSIZE], temp[256], *cp, *cpt;
	DIR *dp, *dp2, *dpt, *dpt2; //dp2 point to last dir_ent
	int find_flag;
	find_flag = 0;

	for (int i = 0; i < 12; ++i){
		get_block(dev, mip->inode.i_block[i], buf);
		dp2 = NULL;
		dp = (DIR*)buf;
		cp = buf;
		while(cp + dp->rec_len <= buf + BLKSIZE && dp->rec_len){
			strncpy(temp, dp->name, dp->name_len);
			temp[dp->name_len] = '\0';
			if (strcmp(temp, name) == 0){
				find_flag = 1;
				dpt = dp;
				dpt2 = dp2;
				cpt = cp;
			}
			dp2 = (DIR *)cp;
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
		if (!find_flag)
			continue;
		// Only when find the entry will get here
		if (dpt->rec_len == BLKSIZE){ // dp is first and only one ent in block
			bdalloc(dev, mip->inode.i_block[i]);
			compact_i_block(mip);
		} else if (dpt->rec_len == buf + BLKSIZE - cpt) { // dp is last ent in block
			dpt2->rec_len += dpt->rec_len;
		} else { // dp is first but not only one || dp is between other ents
			dp2->rec_len += dpt->rec_len;
			memcpy(cpt, cpt + dpt->rec_len, buf + BLKSIZE - (cpt + dpt->rec_len));
		}
		mip->dirty = 1;
		put_block(dev, mip->inode.i_block[i], buf);
		return 0;
	}
	return -1;
}

int jrmdir(char *pathname)
{
	char dname[256], bname[256], buf_d[BLKSIZE], buf_b[BLKSIZE], *cp;
	int ino_d, ino_b; // dir's ino, base's ino
	int bno_d, bno_b;
	MINODE *mip_d, *mip_b;
	DIR *dir_d, *dir_b;

	_dbname(pathname, dname, bname);

	// Make sure dir is removable
	if ((ino_b = getino(dev, pathname)) == -1){
		printf("Error: %s doesn't exist\n", pathname);
		return -1;
	}
	if (!(mip_b = get_inode_from_mem(dev, ino_b)))
		return -1;
	if ((mip_b->inode.i_mode & 0xF000) != 0x4000){
		printf("Error: '%s' is not a dir\n", bname);
		return -1;
	}
	if (mip_b->refCount > 1){
		printf("Error: '%s' is busy\n", bname);
		return -1;
	}
	if (!isDirEmpty(mip_b)){
		printf("Error: '%s': dir is not empty\n", bname);
		return -1;
	}

	// Remove dir_ent from parent's dataBlock
	if ((ino_d = getino(dev, dname)) == -1){
		printf("Error: %s doesn't exist\n", dname);
		return -1;
	}
	if (!(mip_d = get_inode_from_mem(dev, ino_d)))
		return -1;
	rm_child(mip_d, bname);
	mip_d->inode.i_links_count -= 1;
	mip_d->dirty = 1;
	put_inode(mip_d);

	// Deallocate inode and dataBlock
	bdalloc(mip_b->dev, mip_b->inode.i_block[0]);
	idalloc(mip_b->dev, mip_b->ino);
	put_inode(mip_b);
}

