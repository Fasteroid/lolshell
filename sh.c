#include "sh.h"

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <glob.h>

/**
 *  --------
 *  - sh.c -
 *  --------
 *  Authors - Fasteroid & Ratchetlives99
 *  Purpose - Holds the bulk of lolshell
 * 
 *  Warning - Messy.
 * 
 *  Leaks   - Argv arrays passed through exec can't be freed, which may cause leaks.
 *            As noted in class, not freeing stuff that only gets allocated once isn't a big deal.
 *            Due to this (and since Valgrind doesn't technically report them as a leak), I'm relying on the OS to free this memory for me when lolshell exits.
*/

// note: I prefer forward-declaring

/**
 * Prints the lolshell prompt.
 * Async safe so we can use it in the signal handler!
*/
void printprompt() {
    write(STDOUT_FILENO, prompt, strlen(prompt));
    write(STDOUT_FILENO, " [", 3);
    write(STDOUT_FILENO, pwd, strlen(pwd));
    write(STDOUT_FILENO, "] > ", 5);
}

/**
 * Checks for leftover input.
 * Reports so from provided command name if it finds any.
 * @param commandname "commandname: Too many arguments."
 * @return TRUE if leftover input is detected, FALSE otherwise.
*/
int toomanyargs(char *commandname) {
    if (strtok(NULL, " ")) {
        printf("%s: Too many arguments.\n", commandname);
        return 1;
    }
    return 0;
}

/**
 * Checks if the provided strtok result is null.
 * Reports so from provided command name if it is.
 * @param arg pointer to results from strtok
 * @param commandname "commandname: Too many arguments."
 * @return TRUE if arg is null, FALSE otherwise.
*/
int toofewargs(char *arg, char *commandname) {
    if (!arg) {
        printf("%s: Too few arguments.\n", commandname);
        return 1;
    }
    return 0;
}

/**
 * Searches PATH for the first instance of a command and returns it
 * @param command executable name
 * @param pathlist path list
 * @return Executable location.  Must be freed after use!
*/
char *which(char *command, struct pathelement *pathlist) {
    while (pathlist != NULL) {
        char *path = pathlist->element;
        DIR *d;
        struct dirent *dir;

        // thx stackoverflow - https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
        d = opendir(path);
        if (d) {
            dir = readdir(d);  // skip .
            dir = readdir(d);  // skip ..
            while ((dir = readdir(d)) != NULL) {
                if (dir->d_name && strcmp(dir->d_name, command) == 0) {
                    closedir(d);

                    long commandlen = strlen(command);
                    int pathlen = strlen(path);

                    char *finalpath = calloc(1, commandlen + pathlen + 3);  // give enough memory to add the strings
                    strcat(finalpath, path);
                    strcat(finalpath, "/");
                    strcat(finalpath, command);

                    return finalpath;
                }
            }
            closedir(d);
        }
        pathlist = pathlist->next;
    }
    return NULL;  // not found
}

/**
 * Searches PATH for all instances of the provided command
 * and prints their locations to std out.
 * @param command executable name
 * @param pathlist path list
*/
void where(char *command, struct pathelement *pathlist) {
    while (pathlist && pathlist->element) {
        char *path = pathlist->element;
        DIR *d;
        struct dirent *dir;
        d = opendir(path);
        if (d) {
            dir = readdir(d);  // skip .
            dir = readdir(d);  // skip ..
            while ((dir = readdir(d)) != NULL) {
                if (dir->d_name && strcmp(dir->d_name, command) == 0) {
                    printf("where: %s/%s\n", path, command);
                }
            }
            closedir(d);
        }
        pathlist = pathlist->next;
    }
    return;
}

/**
 * Lists all files in a directory
 * @param path directory path
*/
void list(char *path) {
    DIR *d;
    struct dirent *dir;
    int skip = 0;
    printf("%s:\n", path);
    // thx stackoverflow - https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
    d = opendir(path);
    if (d) {
        readdir(d);        // skip .
        dir = readdir(d);  // skip ..
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_name) {                  // keep null safety!
                printf("  %s\n", dir->d_name);  // use \t to indent
            }
        }
        closedir(d);
        printf("\n");
    }
}

/**
 * Prints the environment or a subset of it.
 * @param variable which environment variable to show; pass null to show them all.
 * @param envp pointer to environment array
*/
void printenv(char *variable, char **envp) {
    printf("printenv:\n");
    if (variable) {
        char *value = getenv(variable);
        if (value)
            printf("  %s=%s\n", variable, value);
        else
            printf("  %s=\n", variable);
    } else {
        // thx stackoverflow: https://stackoverflow.com/questions/2085302/printing-all-environment-variables-in-c-c
        for (char **env = envp; *env != 0; env++) {
            printf("  %s\n", *env);
        }
    }
}

/**
 * Executes the provided executable with args and environment
 * @param exe path to executable
 * @param args pointer line read by shell
 * @param envp pointer to environment array
*/
void run(char *exe, char *args, char **envp) {

    pid = fork();

    if (pid < 0) {  // ouch
        printf("exec: Failed\n");
        return;
    }
    status = 1;

    if (!pid) {  // child
    
        // printf("exec: alloc %li: %p-%p (%i args)\n",argnum * sizeof(char *),argv,argv + argnum * sizeof(char *),argnum);

        char *arg = strtok(args, " ");

        glob_t globgogabgalab; // glob man :)

        int append = 0;
        while (arg) {
            errno = 0;
            glob(arg, GLOB_NOCHECK | append, NULL, &globgogabgalab);
            arg = strtok(NULL, " ");
            append = GLOB_APPEND;
        }

        errno = 0;
        // printf("exec: attempt to exec %s\n",exe);
        execve(exe, globgogabgalab.gl_pathv, envp);

        // if death, bail
        printf("exec: execve returned error code %i.\nPlease contact the lolshell developers if you see this message!\n",errno);
        exit(errno);

    } else {  // parent

        int exitcode;
        waitpid(pid, &exitcode, 0);
        if(exitcode){
            printf("exec: Process exited with non-zero exit code %i.\n",exitcode);
        }

    }
}

// ------------------------------------------------------------------------------------------------------------------------------
//                                                      END OF HEADER
// ------------------------------------------------------------------------------------------------------------------------------

/**
 * Actual shell function
*/
int sh(int argc, char **argv, char **envp) {
    int uid, go = 1;
    status = 0;
    pid = getpid();

    prompt = calloc(PROMPTMAX, sizeof(char));             // store shell "name"
    char *commandline = calloc(MAX_CANON, sizeof(char));  // stores the command given
    char *commandtok = calloc(MAX_CANON, sizeof(char));   // place to copy to so we can strtok
    char *tempstring = calloc(MAX_CANON, sizeof(char));   // temporary string manipulation scratchpad
    char *homedir = calloc(MAX_CANON, sizeof(char));      // really don't wanna deal with resizing this

    struct passwd *password_entry;
    struct pathelement *pathlist;

    uid = getuid();
    password_entry = getpwuid(uid);          /* get passwd info */
    strcpy(homedir, password_entry->pw_dir); /* Home directory to start with*/

    // store stuff about starting directory
    char *owd;                                         // *pwd
    if ((owd = getcwd(NULL, PATH_MAX + 1)) == NULL) {  // crash if getting working directory fails
        perror("getcwd");
        exit(2);
    }
    pwd = calloc(strlen(owd) + 1, sizeof(char));  // copy owd to pwd
    memcpy(pwd, owd, strlen(owd));
    strcpy(prompt, "lolshell");
    // prompt[0] = ' '; prompt[1] = '\0';

    /* Put PATH into a linked list */
    pathlist = get_path();  // TODO: don't do this here because it might change during shell execution!

    while (go) {
        /* print your prompt */
        printprompt();

        /* get command line and process */
        char wtf = getchar();  // brace for cursed data
        if (wtf != EOF && wtf != '\n')
            commandline[0] = wtf;  // this command is valid enough to pass
        else {
            if (wtf == EOF)
                write(STDOUT_FILENO, "\n", 2);  // printf didn't work here
            continue;
        }

        fgets(commandline + 1, MAX_CANON - 1, stdin);  // grab the remaining characters

        commandline[strlen(commandline) - 1] = '\0';  // get rid of stupid newline, was causing problems
        memcpy(commandtok, commandline, MAX_CANON);

        char *arg = strtok(commandtok, " ");

        if (!arg) {  // don't segfault on empty input
            continue;
        }

        // ------------- exit -------------
        if (strcmp(arg, "exit") == 0) {
            //free_path(pathlist);
            printf("lol\n");
            exit(0);
        }

        // ------------- which -------------
        if (strcmp(arg, "which") == 0) {
            arg = strtok(NULL, " ");
            if (toofewargs(arg, "which")) continue;
            while (arg) {
                char *whichcommand = which(arg, pathlist);
                if (whichcommand) {
                    printf("which: %s\n", whichcommand);
                }
                free(whichcommand);
                arg = strtok(NULL, " ");
            }
            continue;
        }

        // ------------- where -------------
        if (strcmp(arg, "where") == 0) {
            arg = strtok(NULL, " ");
            if (toofewargs(arg, "where")) continue;
            while (arg) {
                where(arg, pathlist);
                arg = strtok(NULL, " ");
            }
            continue;
        }

        // ------------- list -------------
        if (strcmp(arg, "list") == 0) {
            arg = strtok(NULL, " ");
            if (!arg) {  // no args
                list(pwd);
                continue;
            }
            while (arg) {
                list(arg);
                arg = strtok(NULL, " ");
            }
            continue;
        }

        // ------------- kill -------------
        char *endptr;  // needed for strtol
        if (strcmp(arg, "kill") == 0) {
            arg = strtok(NULL, " ");
            if (toofewargs(arg, "kill")) continue;
            int sig = 9;
            if (*arg == '-') {
                errno = 0;                           // clear error
                sig = strtol(arg + 1, &endptr, 10);  // +1 to skip the dash
                if (errno) {
                    printf("kill: Failed to parse signal '%s'.\n", arg);
                    continue;
                }
                arg = strtok(NULL, " ");
                if (!arg) {  // missing pid
                    printf("kill: Missing pid to send signal to.\n");
                    continue;
                }
            }
            errno = 0;  // clear error
            pid_t killpid = strtol(arg, &endptr, 10);
            if (errno) {
                printf("kill: Failed to parse pid '%s'.\n", arg);
                continue;
            }
            if (toomanyargs("kill")) continue;
            printf("kill: Sending signal %i to pid %i...", sig, killpid);
            kill(killpid, sig);
            continue;
        }

        // ------------- cd -------------
        if (strcmp(arg, "cd") == 0) {
            arg = strtok(NULL, " ");
            if (arg) {
                if (toomanyargs("cd")) continue;
                if (strcmp(arg, "-") == 0) {
                    strcpy(arg, owd);  // dump this in here if we wanna go back
                    chdir(arg);
                    strcpy(owd, pwd);
                    strcpy(pwd, arg);
                } else {
                    // thx stackoverflow - https://stackoverflow.com/questions/1563168/example-of-realpath-function-in-c
                    char realcwd[MAX_CANON];  // buffers are fun!
                    realpath(arg, realcwd);   // parse what we're given with realcwd so we don't do something dumb like "lolshell [..] >"

                    if (realcwd) {
                        DIR *d;
                        d = opendir(realcwd);
                        if (d) {
                            closedir(d);
                            chdir(realcwd);
                            strcpy(owd, pwd);
                            strcpy(pwd, realcwd);
                        } else {
                            printf("cd: '%s' is not a directory.\n", realcwd);
                        }
                    } else {
                        printf("cd: '%s' is inaccesible.\n", realcwd);
                    }
                }
            } else {  // easy
                chdir(homedir);
                strcpy(owd, pwd);
                strcpy(pwd, homedir);
            }
            continue;
        }

        // ------------- prompt -------------
        if (strcmp(arg, "prompt") == 0) {
            arg = strtok(NULL, " ");
            if (!arg) {
                printf("prompt: Enter new shell name > ");
                fgets(prompt, PROMPTMAX, stdin);
                prompt[strlen(prompt) - 1] = '\0';  // get rid of stupid newline AGAIN
                continue;
            } else {
                if (toomanyargs("prompt")) continue;
                strncpy(prompt, arg, PROMPTMAX);
                continue;
            }
        }

        // ------------- printenv -------------
        if (strcmp(arg, "printenv") == 0 || strcmp(arg, "getenv") == 0) {
            arg = strtok(NULL, " ");
            if (arg && toomanyargs("printenv")) continue;
            printenv(arg, envp);
            continue;
        }
        
        // ------------- setenv -------------
        if (strcmp(arg, "setenv") == 0) {
            arg = strtok(NULL, " ");
            if (!arg) {  // no args
                printenv(arg, envp);
                continue;
            } else {  // 1 or 2 args

                char *variable = arg;
                arg = strtok(NULL, " ");

                // handle HOME change
                if (strcmp(variable, "HOME") == 0) {
                    char realcwd[MAX_CANON];
                    realpath(arg, realcwd);  // CHECK IF THIS IS VALID (and parse it) BEFORE WE SET IT!
                    if (realcwd) {
                        DIR *d;
                        d = opendir(realcwd);
                        if (!d) {
                            printf("setenv: WARNING: Invalid HOME directory '%s' provided. Not setting.\n", arg);
                            continue;
                        }
                        closedir(d);
                    } else {
                        printf("setenv: WARNING: Invalid HOME directory '%s' provided. Not setting.\n", arg);
                        continue;
                    }
                    strcpy(homedir, arg);
                }

                if (arg) {
                    if (toomanyargs("setenv")) continue;
                    setenv(variable, arg, 1);
                } else {
                    setenv(variable, "", 1);
                }

                // handle PATH change
                if (strcmp(variable, "PATH") == 0) {
                    pathlist = get_path();  // no need to free the old one; get_path() now does this for us!
                }

                printf("setenv: Value changed.\n");
                continue;
            }
        }

        // ------------- pid -------------
        if (strcmp(arg, "pid") == 0) {
            if (toomanyargs("pid")) continue;
            printf("pid: %i\n", getpid());
            continue;
        }

        char *exe = which(arg, pathlist);
        if (exe && access(exe, uid) == 0) {
            run(exe, commandline, envp);
        } else if (access(arg, uid) == 0) {  // maybe they passed an absolute?
            run(arg, commandline, envp);
        }
        else{
            printf("exec: Command not found.\n");
        }
    }
    return 0;
}
