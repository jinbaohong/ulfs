#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define NAMESIZ 64

int get_cmd_index(char *command);
int mkdir(char *pathname);


struct node {
	char _name[NAMESIZ];
	char _type; // 'D'=directory;'F'=file
	struct node *_chld, *_sib, *_prt;
};

struct node *root, *cwd;
char line[128];
char command[16], pathname[64];
char dname[64], bname[64];
char *cmd[] = {"mkdir", "rmdir", "ls", "cd", "pwd", "creat",
			   "rm", "reload", "save", "menu", "quit", NULL};
// int (*handlers[])(char*) = {mkdir, rmdir, ls, cd, pwd, creat,
// 			   				rm, reload, save, menu, quit, NULL};
int (*handlers[])(char*) = {mkdir, NULL};



int main(int ac, char const *av[])
{
	int hasPath, index, r;

	/* Initialize */
	root = calloc(sizeof(struct node), 1);
	strcpy(root->_name, "/");
	root->_type = 'D';
	cwd = root;

	while (1) {
		printf("%s$ ", cwd->_name); // Prompt
		fgets(line, 128, stdin); // Get input from stdin
		line[strlen(line)-1] = '\0';
		hasPath = sscanf(line, "%s %s", command, pathname); // Parse input as cmd and path

		/* Parse command */
		if ((index = get_cmd_index(command)) == -1){
			printf("Error: command '%s' doesn't exist\n", command);
			continue;
		}
		r = handlers[index](pathname);
		if (r == -1)
			printf("Error\n");
	}
	return 0;
}

int mkdir(char *pathname)
{
	struct node *tmp;

	tmp = calloc(sizeof(struct node), 1);
	strcpy(tmp->_name, pathname);
	tmp->_type = 'D';
	tmp->_prt = cwd;
	if (cwd->_chld) // tmp is not the first child.
		cwd->_chld->_sib = tmp;
	else // tmp is the first child
		cwd->_chld = tmp;
	
	return 0;
}


int get_cmd_index(char *command)
{
	for (int i = 0; cmd[i] != NULL ; i++)
		if (strcmp(command, cmd[i]) == 0)
			return i;
	return -1; // failed to search.
}
