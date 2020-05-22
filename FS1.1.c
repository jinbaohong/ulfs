#include "type.h"
#include "global.c"
#include "util.c"

// global variables
#include "command.c"

int fs_init()
{
	int i,j;
	for (i=0; i<NMINODE; i++)
		minode[i].refCount = 0;
	for (i=0; i<NMTABLE; i++)
		mtable[i].dev = 0;
	for (i=0; i<NOFT; i++)
		oft[i].refCount = 0;
	for (i=0; i<NPROC; i++){
		proc[i].status = READY;
		proc[i].pid = i;
		proc[i].uid = i;
		for (j=0; j<NFD; j++)
			proc[i].fd[j] = 0;
		proc[i].next = &proc[i+1];
	}
	proc[NPROC-1].next = &proc[0];
	running = &proc[0];
}

int mount_root(char *rootdev)
{
	int i;
	MTABLE *mp;
	SUPER *sp = calloc(sizeof(SUPER), 1);
	GD *gp;
	char buf[BLKSIZE];

	dev = open(rootdev, O_RDWR);
	if (dev < 0){
		printf("panic : can’t open root device\n");
		exit(1);
	}
	/* get super block of rootdev */
	if (check_ext2(dev) == -1)
		printf("Error: device %s is not EXT2\n", rootdev), exit(1);

	// fill mount table mtable[0] with rootdev information
	mp = &mtable[dev]; // use mtable[0]
	mp->dev = dev;
	// copy super block info into mtable[0]
	get_sp(dev, sp);
	ninodes = mp->ninodes = sp->s_inodes_count;
	nblocks = mp->nblocks = sp->s_blocks_count;
	strcpy(mp->devName, rootdev);
	strcpy(mp->mntName, "/");
	get_block(dev, 2, buf);
	gp = (GD *)buf;
	bmap = mp->bmap = gp->bg_block_bitmap;
	imap = mp->imap = gp->bg_inode_bitmap;
	iblock = mp->iblock = gp->bg_inode_table;
	printf("bmap=%d imap=%d iblock=%d\n", bmap, imap, iblock);
	// call iget(), which inc minode’s refCount
	root = get_inode_from_mem(dev, 2); // get root inode
	root->mounted = 1; // I think this should be set
	mp->mntDirPtr = root; // double link
	root->mntPtr = mp;
	// set proc CWDs
	for (i=0; i<NPROC; i++) // set proc’s CWD
		proc[i].cwd = get_inode_from_mem(dev, 2); // each inc refCount by 1
	printf("mount : %s mounted on / \n", rootdev);
	return 0;
}

int main(int argc, char *argv[ ])
{
	char line[128], cmd[16], pathname[64], pathname2[64];
	if (argc > 1)
		rootdev = argv[1];
	fs_init();
	mount_root(rootdev);
	while(1){
		memset(pathname, 0, 64);
		printf("P%d running: ", running->pid);
		printf("input command : ");
		fgets(line, 128, stdin);
		line[strlen(line)-1] = 0;
		if (line[0]==0)
			continue;
		sscanf(line, "%s %s %s", cmd, pathname, pathname2);
		if (!strcmp(cmd, "ls"))
			jls(pathname);
		if (!strcmp(cmd, "cd"))
			jchdir(pathname);
		if (!strcmp(cmd, "pwd"))
			jpwd(running->cwd);
		if (!strcmp(cmd, "mkdir"))
			jmkdir(pathname);
		if (!strcmp(cmd, "creat"))
			jcreat(pathname);
		if (!strcmp(cmd, "rmdir"))
			jrmdir(pathname);
		if (!strcmp(cmd, "link"))
			jlink(pathname, pathname2);
		if (!strcmp(cmd, "unlink"))
			junlink(pathname);
		if (!strcmp(cmd, "quit"))
			jquit();
		minodes_print();
	}
}

