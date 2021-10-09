/*
    get_path.c
    Ben Miller

    Just a little sample function that gets the PATH env var, parses it and
    puts "components" into a linked list, which is returned.
*/
#include "get_path.h"

// keep track of these locally to make usage easier
char *pathstring;
struct pathelement *pathlistlocal;

struct pathelement *get_path()
{

    if(pathlistlocal){ // auto free the last pathlistlocal
        free_path();
    }

    /* path is a copy of the PATH and p is a temp pointer */
    char *p;

    /* tmp is a temp point used to create a linked list and pathlistlocal is a
         pointer to the head of the list */
    struct pathelement *tmp = NULL;

    p = getenv("PATH");	/* get a pointer to the PATH env var.
                 make a copy of it, since strtok modifies the
                 string that it is working with... */
    pathstring = malloc((strlen(p)+1)*sizeof(char));	/* use malloc(3) */
    // printf("alloc: %p\n",pathstring);
    strncpy(pathstring, p, strlen(p));
    pathstring[strlen(p)] = '\0';

    p = strtok(pathstring, ":"); 	/* PATH is : delimited */
    do				/* loop through the PATH */
    {				/* to build a linked list of dirs */
        if ( !pathlistlocal )		/* create head of list */
        {
            tmp = calloc(1, sizeof(struct pathelement));
            // printf("alloc: %p\n",tmp);
            pathlistlocal = tmp;
        }
        else			/* add on next element */
        {
            tmp->next = calloc(1, sizeof(struct pathelement));
            // printf("alloc: %p\n",tmp->next);
            tmp = tmp->next;
        }
        tmp->element = p;	
        tmp->next = NULL;
    } while ( p = strtok(NULL, ":") );
    return pathlistlocal;
} /* end get_path() */

/**
 * Frees the pathelement linked list retrieved by get_path()
*/
void free_path(){
    while (pathlistlocal != NULL) {
        struct pathelement *delete = pathlistlocal;
        pathlistlocal = pathlistlocal->next;
        free(delete);
        // printf("free: %p\n",delete);
    }
    free(pathstring); // don't forget this!
    // printf("free: %p\n",pathstring);
}
