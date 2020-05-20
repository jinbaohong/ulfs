MINODE minode[NMINODE];
MTABLE mtable[NMTABLE];
OFT oft[NOFT];
PROC proc[NPROC];
PROC *running;
int ninodes, nblocks, bmap, imap, iblock;
int dev;
char *rootdev = "mydisk"; // default root_device
MINODE *root;


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

int minodes_print()
{
	printf("********MINODE**[dev ino refCount]*******\n");
	for (int i = 0; i < 10; i++) //NMINODE
		printf("[%d %d %d]->", minode[i].dev, minode[i].ino, minode[i].refCount);
	printf("\n");
	printf("running->cwd->ino = %d\n", running->cwd->ino);
	printf("*****************************************\n");

}