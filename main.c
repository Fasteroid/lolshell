#include "sh.h"
#include <signal.h>
#include <stdio.h>

void sig_handler(int sig){
    switch (sig){
        case SIGTERM:
            // thx stackoverflow: https://stackoverflow.com/questions/16891019/how-to-avoid-using-printf-in-a-signal-handler
            write(STDOUT_FILENO,"\n",2);
            if(!pid) exit(0);
        break;

        case SIGINT:
            write(STDOUT_FILENO,"\n",2);
            printprompt();
            if(!pid) exit(0);
        break;

        case SIGTSTP:
            write(STDOUT_FILENO,"\n",2);
            printprompt();
            if(!pid) exit(0);
        break;
        
        default:
        break;
    }
    signal(SIGINT,sig_handler);
}

int main( int argc, char **argv, char **envp )
{
    signal(SIGINT,sig_handler);
    signal(SIGTSTP,sig_handler);
    return sh(argc, argv, envp);
}

// I prefer to put functions at the beginning
