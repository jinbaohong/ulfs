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
void _dbname(char *pathname);
int _get_cmd_index(char *command);
int tokenize(char *pathname);
void _print_full_path(struct node *nd);
struct node *_get_node_by_pathname(char *pathname);




struct node *root, *cwd;
char line[128];
char command[16], pathname[64];
char dname[64], bname[64];
char *cmd[] = {"mkdir", "rmdir", "ls", "cd", "pwd", "creat",
			   "rm", "reload", "save", "menu", "quit", NULL};
// int (*handlers[])(char*) = {mkdir, rmdir, ls, cd, pwd, creat,
// 			   				rm, reload, save, menu, quit, NULL};
int (*handlers[])(char*) = {mkdir, rmdir, ls, cd, pwd, NULL};



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
		printf("ulfs:");
		_print_full_path(cwd);
		printf("$ "); // Prompt
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

int pwd(char *pathname)
{
	_print_full_path(cwd);
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
		/* Now, tmp is the last one of tar */
		tmp->_sib = tar->_sib;
	}

	/* Second, free tar */
	free(tar);

	return 0;
}

int mkdir(char *pathname)
{
	struct node *new, *latestSib;

	new = calloc(sizeof(struct node), 1);
	strcpy(new->_name, pathname);
	new->_type = 'D';
	new->_prt = cwd;

	if (cwd->_chld) { // new is not the first child.
		latestSib = cwd->_chld;
		while (latestSib->_sib)
			latestSib = latestSib->_sib;
		latestSib->_sib = new;
	}
	else // new is the first child
		cwd->_chld = new;
	
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
/* Side effect: global's dname and bname will be filled in. */
{
	char tmp[128];

	strcpy(tmp, pathname);
	strcpy(dname, dirname(tmp));
	strcpy(tmp, pathname);
	strcpy(bname, basename(tmp));
}

void _print_full_path(struct node *nd)
{
	if (nd != nd->_prt) // nd is not root.
		_print_full_path(nd->_prt);
	else {
		printf("/");
		return;
	}
	printf("%s/", nd->_name);	
	return;
}

struct node *_get_node_by_pathname(char *pathname)
{
	struct node *tmp;
	char *s, tmp_str[64];

	if (!strcmp("..", pathname)) {
		tmp = cwd->_prt;
	} else { 
		strcpy(tmp_str, pathname); // strtok() will modify original string.
		tmp = *pathname == '/' ? root : cwd; // Path is absolute or relative?
		s = strtok(tmp_str, "/"); // first call to strtok()
		while(s){
			tmp = tmp->_chld;		
			while (tmp != NULL && strcmp(tmp->_name, s))
				tmp = tmp->_sib;
			s = strtok(0, "/"); // call strtok() until it returns NULL
		}
	}
	return tmp;
}
