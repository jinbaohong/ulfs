#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#define NAMESIZ 64

struct node {
	char _name[NAMESIZ];
	char _type; // 'D'=directory;'F'=file
	struct node *_chld, *_sib, *_prt;
};

int mkdir(char *pathname);
int rmdir(char *pathname);
int ls(char *pathname);
int cd(char *pathname);
int pwd(char *pathname);
int creat(char *pathname);
int rm(char *pathname);
int reload(char *pathname);
int save(char *pathname);
int menu(char *pathname);
int quit(char *pathname);



void _dbname(char *pathname);
int _get_cmd_index(char *command);
int tokenize(char *pathname);
void _print_full_path(struct node *nd, FILE *fp);
struct node *_get_node_by_pathname(char *pathname);
void _save_by_node(struct node *tar, FILE *fp);




struct node *root, *cwd;
char line[128];
char command[16], pathname[64];
char dname[64], bname[64];
char *cmd[] = {"mkdir", "rmdir", "ls", "cd", "pwd", "creat",
			   "rm", "reload", "save", "menu", "quit", NULL};
int (*handlers[])(char*) = {mkdir, rmdir, ls, cd, pwd, creat,
			   				rm, reload, save, menu, quit, NULL};
// int (*handlers[])(char*) = {mkdir, rmdir, ls, cd, pwd, NULL};



int main(int ac, char const *av[])
{
	int hasPath, index, r;

	/* Initialize */
	root = calloc(sizeof(struct node), 1);
	strcpy(root->_name, "/");
	root->_type = 'D';
	root->_prt = root;
	root->_sib = root;
	cwd = root;

	while (1) {
		printf("ulfs:"); _print_full_path(cwd, stdout);	printf("$ "); // Prompt
		fgets(line, 128, stdin); // Get input from stdin
		line[strlen(line)-1] = '\0';
		hasPath = sscanf(line, "%s %s", command, pathname); // Parse input as cmd and path
		if (hasPath == 1)
			memset(pathname, 0, 64);

		/* Parse command */
		if ((index = _get_cmd_index(command)) == -1){
			printf("Error: command '%s' doesn't exist\n", command);
			continue;
		}
		r = handlers[index](pathname);
		if (r == -1)
			printf("Error\n");
	}
	return 0;
}

int creat(char *pathname)
{
	return 0;
}
int rm(char *pathname)
{
	return 0;
}

int reload(char *pathname)
{
	FILE *fp;
	char buf[256];

 	fp = fopen("fsimg", "r");
	fgets(buf, 256, fp); // Consume first line (i.e. D /).
	while (fgets(buf, 256, fp)) {
		if (*buf == 'D'){
			buf[strlen(buf)-1] = 0;
			mkdir(buf+2);
		}
	}
	return 0;
}

int save(char *pathname)
{
	FILE *fp = fopen("fsimg", "w+");

	/* Preorder traverse tree started from root. */
	_save_by_node(root, fp);
	fclose(fp);
}

void _save_by_node(struct node *tar, FILE *fp)
{
	/* Boundary condition */
	if (!tar)
		return;

	/* Preorder traverse tree */
	/* D node */
	fprintf(fp, "%c ", tar->_type);
	_print_full_path(tar, fp);
	fprintf(fp, "\n");
	/* L node */
	_save_by_node(tar->_chld, fp);
	/* R node */
	if (tar != root) // root->_sib == root, so we should avoid infinite recursion.
		_save_by_node(tar->_sib, fp);

	return;
}

int menu(char *pathname)
{
	return 0;
}
int quit(char *pathname)
{
	return 0;
}


int pwd(char *pathname)
{
	_print_full_path(cwd, stdout);
	printf("\n");
	return 0;
}

int ls(char *pathname)
{
	struct node *i;

	i = (!strcmp(pathname, "")) ? cwd->_chld : _get_node_by_pathname(pathname)->_chld;
	for (; i != NULL; i = i->_sib)
		printf("%s  ", i->_name);
	printf("\n");

	return 0;
}

int rmdir(char *pathname)
{ // Free all node under target, i.e. rm -R
	struct node *tar, *tmp;

	if (!strcmp(pathname, "/")) {
		printf("Error: can't remove root\n");
		return -1;
	}

	if ((tar = _get_node_by_pathname(pathname)) == NULL) {
		printf("Error: %s doesn't exist\n", pathname);
		return -1;
	}

	/* First, hanle tar's context (i.e. parent and sibling) */
	if (tar->_prt->_chld == tar) // tar is the first child.
		if (tar->_sib)
			tar->_prt->_chld = tar->_sib;
	else { // tar is not the first child.
		tmp = tar->_prt->_chld;
		while (tmp->_sib != tar)
			tmp = tmp->_sib;
		/* Now, tmp is the precedence of tar */
		tmp->_sib = tar->_sib;
	}

	/* Second, free tar */
	free(tar);

	return 0;
}

int mkdir(char *pathname)
{
	struct node *new, *latestSib, *tmp;

	/* tmp is new's parent */
	_dbname(pathname);
	if ((tmp = _get_node_by_pathname(dname)) == NULL) {
		printf("Error: %s doesn't exit\n", dname);
		return -1;
	}

	new = calloc(sizeof(struct node), 1);
	strcpy(new->_name, bname);
	new->_type = 'D';
	new->_prt = tmp;

	if (tmp->_chld) { // new is not the first child.
		latestSib = tmp->_chld;
		while (latestSib->_sib)
			latestSib = latestSib->_sib;
		latestSib->_sib = new;
	}
	else // new is the first child
		tmp->_chld = new;
	
	return 0;
}

int cd(char *pathname)
{
	if ((cwd = _get_node_by_pathname(pathname)) == NULL) {
		printf("Error: %s doesn't exist\n", pathname);
		return -1;
	}
	return 0;
}


int _get_cmd_index(char *command)
{
	for (int i = 0; cmd[i] != NULL ; i++)
		if (strcmp(command, cmd[i]) == 0)
			return i;
	return -1; // failed to search.
}

void _dbname(char *pathname)
/* Side effect: global's dname and bname will be filled in.
 * Warning: You should clean (e.g. remove \n) pathname first
 */
{
	char tmp[128];

	strcpy(tmp, pathname);
	strcpy(dname, dirname(tmp));
	strcpy(tmp, pathname);
	strcpy(bname, basename(tmp));
}

void _print_full_path(struct node *nd, FILE *fp)
{
	if (nd != nd->_prt) // nd is not root.
		_print_full_path(nd->_prt, fp);
	else {
		fprintf(fp, "/");
		return;
	}
	fprintf(fp, "%s/", nd->_name);	
	return;
}

struct node *_get_node_by_pathname(char *pathname)
{
	struct node *tmp;
	char *s, tmp_str[64];

	if (!strcmp("..", pathname)) {
		tmp = cwd->_prt;
	} else if (!strcmp(".", pathname)) {
		tmp = cwd;
	} else { 
		strcpy(tmp_str, pathname); // strtok() will modify original string.
		tmp = *pathname == '/' ? root : cwd; // Path is absolute or relative?
		s = strtok(tmp_str, "/"); // first call to strtok()
		while(s){
			tmp = tmp->_chld;		
			while (tmp != NULL && strcmp(tmp->_name, s))
				tmp = tmp->_sib;
			if (!tmp)
				break;
			s = strtok(0, "/"); // call strtok() until it returns NULL
		}
	}
	return tmp;
}
