
#include "get_path.h"

int pid;
char *pwd, *prompt;

int sh( int argc, char **argv, char **envp);
char *which(char *command, struct pathelement *pathlist);
void where(char *command, struct pathelement *pathlist);
void list ( char *dir );
void printenv(char **envp);
void dircat(char *str1, char *str2);
void printprompt();

#define PROMPTMAX 32
#define MAXARGS 10
