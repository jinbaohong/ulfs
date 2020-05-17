#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <ext2fs/ext2_fs.h>

#define FILE_NAME_SIZ 256
#define BLKSIZE 1024
typedef struct ext2_group_desc GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode INODE;
typedef struct ext2_dir_entry_2 DIR;


int get_block(int fd, int blk, char *buf)
{
	lseek(fd, blk*BLKSIZE, SEEK_SET);
	read(fd, buf, BLKSIZE);
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

int get_gpd(int fd, SUPER *sp, GD *gd)
{
	lseek(fd, (sp->s_first_data_block+1)*BLKSIZE, SEEK_SET);
	read(fd, gd, sizeof(GD));
	return 0;
}


int get_inode(int fd, GD *gd, int inode_no, INODE *inode)
{ // ip is inode table base address
	lseek(fd, (gd->bg_inode_table)*BLKSIZE + (inode_no-1)*sizeof(INODE), SEEK_SET);
	read(fd, inode, sizeof(INODE));
	return 0;
}

int search(int fd, INODE *inode, char *name)
{ // Search dir_ent's inode by name in inode
	char buf[BLKSIZE], temp[256], *cp;
	DIR *dp;

	for (int i = 0; i < 12; i++)
	{ // Assume user only use direct block
		get_block(fd, inode->i_block[i], buf);

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


int dir_print(int fd, GD *gd, INODE *inode_dir)
{ // Search dir_ent's inode by name in inode
	char buf[BLKSIZE], temp[256], *cp;
	DIR *dp;
	INODE *inode = calloc(sizeof(INODE), 1);

	for (int i = 0; i < 12; i++)
	{ // Assume user only use direct block
		get_block(fd, inode_dir->i_block[i], buf);

		dp = (DIR*)buf;
		cp = buf;
		while(cp < buf + BLKSIZE && dp->rec_len){
			strncpy(temp, dp->name, dp->name_len);
			temp[dp->name_len] = '\0';
			get_inode(fd, gd, dp->inode, inode);
			inode_print(inode, dp->inode, temp);
			cp += dp->rec_len;
			dp = (DIR *)cp;	
		}
	}
}

int show(int fd, char *path)
{
	char *s, tmp_str[FILE_NAME_SIZ];
	int firstdata, inodesize, blksize, inode_no;
	SUPER *sp = calloc(sizeof(SUPER), 1);
	GD *gd = calloc(sizeof(GD), 1);
	INODE *root = calloc(sizeof(INODE), 1);
	INODE *inode = calloc(sizeof(INODE), 1);

	// Get sp
	get_sp(fd, sp);
	firstdata = sp->s_first_data_block;
	inodesize = sp->s_inode_size;
	blksize = 1024*(1<<sp->s_log_block_size);
	
	// Get group descriptor	
	get_gpd(fd, sp, gd);

	// Get root inode in inode-table
	get_inode(fd, gd, 2, root);

	inode = root;
	strcpy(tmp_str, path); // strtok() will modify original string.
	// tmp = *pathname == '/' ? root : cwd; // Path is absolute or relative?
	s = strtok(tmp_str, "/"); // first call to strtok()
	while(s){ // s is token
		if ((inode_no = search(fd, inode, s)) == -1)
			printf("Error: can't find %s\n", s), exit(1);
		// printf("Find %s is at inode %d\n", s, inode_no);

		// Update inode and s
		get_inode(fd, gd, inode_no, inode);
		if (inode->i_mode >> 12 != 4)
			break; // if inode is not dir, then break
		s = strtok(0, "/"); // call strtok() until it returns NULL
	}
	if (inode->i_mode >> 12 == 4) // when inode is DIR
		dir_print(fd, gd, inode);
	else // when inode is FILE
		inode_print(inode, inode_no, path);
	return 0;
}




int main(int ac, char *av[])
{
	int fd;

	if (ac != 3)
		printf("Usage: %s device absolute-path\n", av[0]), exit(1);

	if ((fd = open(av[1], O_RDONLY)) == -1)
		perror("open"), exit(1);

	if (check_ext2(fd) == -1)
		printf("Error: device %s is not EXT2\n", av[1]), exit(1);

	show(fd, av[2]);

	return 0;
}
