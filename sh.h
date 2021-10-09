
#include "get_path.h"

int pid;
char *pwd, *prompt;

int sh( int argc, char **argv, char **envp);
void printprompt();

#define PROMPTMAX 32
#define MAXARGS 10
