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

int jquit(void);
