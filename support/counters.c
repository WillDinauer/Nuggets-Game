/* 
 * counters.c - CS50 'counters' module
 *
 * largely mirrors the structure of the provided 
 * CS50 'bag' module, created by David Kotz
 *
 * see counters.h for more information.
 *
 * William Dinauer, Dartmouth CS50 Winter 2021
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "counters.h"
#include "memory.h"


/**************** local types ****************/
typedef struct cntnode {
    int key;        // any given integer key
    int count;      // the count for the given key
    struct cntnode *next;   // the next node (key/count pair) in the list
} cntnode_t;


/**************** global types ****************/
typedef struct counters {
    struct cntnode *head;   // the first item in the linked list
} counters_t;

/**************** local functions ****************/
/* not visible outside this file */
static cntnode_t *cntnode_new(int key);

/**************** counters_new() ****************/
/* see counters.h for description */
counters_t *counters_new(void) 
{
    // allocate memory for the data structure
    counters_t *ctrs = count_malloc(sizeof(counters_t));
    if (ctrs == NULL) {
        return NULL;    //error allocating ctrs
    } else {
        ctrs->head = NULL;
        return ctrs;
    }
}

/**************** counters_add() ****************/
/* see counters.h for description */
int counters_add(counters_t *ctrs, const int key)
{
    if (ctrs != NULL && key >= 0) {
        if (ctrs->head == NULL) {
            // for an empty counters, create a new node as the head
            cntnode_t *new = cntnode_new(key);
            if (new != NULL) {
                ctrs->head = new;
                return 1;
            }
        } else {
            for (cntnode_t *node = ctrs->head; node != NULL; node = node->next) {
                if (node->key == key){              
                    //found a match; increment count
                    return ++node->count;
                } else {
                    if (node->next == NULL){        
                        //at the last node, no matches
                        cntnode_t *new = cntnode_new(key);
                        if (new != NULL) {
                            //insert at the head
                            new->next = ctrs->head;
                            ctrs->head = new;
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

/**************** counters_get() ****************/
/* see counters.h for description */
int counters_get(counters_t *ctrs, const int key) 
{
    if (ctrs != NULL && key >= 0) {
        // search through all nodes, starting at the head
        for (cntnode_t *node = ctrs->head; node != NULL; node = node->next) {
            if (node->key == key) {     // match found
                return node->count;     // return the count
            }
        }
    }
    return 0;
}

/**************** counters_set() ****************/
/* see counters.h for description */
bool counters_set(counters_t *ctrs, const int key, const int count)
{
    if (ctrs != NULL && key >= 0 && count>=0) {
        if (ctrs->head == NULL) {
            // empty counters
            cntnode_t *new = cntnode_new(key);
            if (new != NULL) {      // checks for error allocating memory
                new->count = count; // insert at the head
                ctrs->head = new;
                return true;
            } 
            return false;
        } else {
            // traverse the entire linked list
            for (cntnode_t *node = ctrs->head; node != NULL; node = node->next) {
                if (node->key == key) {     // match found
                    node->count = count;    // update count
                    return true;            
                }
                if (node->next == NULL) {   // reached the end, no matches
                    //create a new node; insert at the head
                    cntnode_t *new = cntnode_new(key);
                    if (new != NULL) {
                        new->count = count; // update the count
                        new->next = ctrs->head;
                        ctrs->head = new;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

/**************** counters_print() ****************/
/* see counters.h for description */
void counters_print(counters_t *ctrs, FILE *fp) 
{
    if (fp != NULL) {
        if (ctrs != NULL) {
            fputc('{', fp);
            // print in format key=count for each node
            for (cntnode_t *node = ctrs->head; node !=NULL; node = node->next) {
                fprintf(fp, "%d=%d", node->key, node->count);
                if (node->next != NULL){
                    // commas between nodes
                    fputc(',', fp);
                } 
            }
            fputc('}', fp);
            fputc('\n', fp);
        } else {
            fputs("(null)\n", fp);
        }
    }
}

/**************** counters_iterate() ****************/
/* see counters.h for description */
void counters_iterate(counters_t *ctrs, void *arg, 
                      void (*itemfunc)(void *arg, 
                                       const int key, const int count))
{
    if (ctrs != NULL && itemfunc != NULL) {
        // call the given function for every node in the counter
        for (cntnode_t *node = ctrs->head; node != NULL; node = node->next) {
            (*itemfunc)(arg, node->key, node->count);
        }
    }
}

/**************** counters_delete() ****************/
/* see counters.h for description */
void counters_delete(counters_t *ctrs)
{
    if (ctrs != NULL) {
        for (cntnode_t *node = ctrs->head; node!=NULL; ) {
            cntnode_t *next = node->next;   // save the next node
            count_free(node);               // free the current node
            node = next;                    // traverse to the next node
        }
        count_free(ctrs); // free the overall counters data structure
    }
}

/**************** cntnode_new() ****************/
/* allocates a counters node for insertion into 
 * the linked list */
static cntnode_t *cntnode_new(int key)
{
    cntnode_t *node = count_malloc(sizeof(cntnode_t));
    if (node == NULL) {
        // error allocating memory for node; return error
        return NULL;
    } else {
        // initialize values, count set to 1
        node->key = key;
        node->count = 1;
        node->next = NULL;
        return node;
    }
}
