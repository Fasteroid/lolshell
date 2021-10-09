/* 
  get_path.h
  Ben Miller + Caleb Brownstein's 'free_path'
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/**
 * Retrieves and sets up the path element linked list
 * @returns pointer to first element of linked list
*/
struct pathelement *get_path();

/**
 * Frees a pathelement linked list retrieved by get_path()
*/
void free_path();

struct pathelement
{
    char *element;			        /* a dir in the path */
    struct pathelement *next;		/* pointer to next node */
};

