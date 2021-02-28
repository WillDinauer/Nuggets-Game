/* 
 * set.c - CS50 'set' module
 *
 * largely mirrors the structure of the provided 
 * CS50 'bag' module, created by David Kotz
 *
 * see set.h for more information.
 *
 * William Dinauer, Dartmouth CS50 Winter 2021
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "string.h"
#include "set.h"
#include "memory.h"

/**************** local types ****************/
typedef struct setnode {
    char *key;          // stores a unique string key
    void *item;         // the item to be stored
    struct setnode *next;   // the next node in the list
} setnode_t;

/**************** global types ****************/
typedef struct set {
    struct setnode *head;   // the front of the linked list
} set_t;

/**************** local functions ****************/
/* not visible outside this file */
static setnode_t *setnode_new(const char *key, void *item);

/**************** set_new() ****************/
/* see set.h for description */
set_t *set_new(void) {
    set_t *set = count_malloc(sizeof(set_t));
    if (set == NULL) {
        // error allocating memory
        return NULL;
    } else {
        set->head = NULL;
        return set;
    }
}

/**************** set_insert() ****************/
/* see set.h for description */
bool set_insert(set_t *set, const char *key, void *item) {
    if (set != NULL && key != NULL && item != NULL) {
       if (set->head == NULL) {
           // empty set; create a new node as the head     
           setnode_t *new = setnode_new(key, item);
           if (new != NULL) {
               set->head = new;
               return true;
           }
       } else {
           // traverse through the entire set
           for (setnode_t *node = set->head; node != NULL; node = node->next) {
               if (strcmp(node->key, key) == 0) {
                   // return false if the key is already in the set; no duplicates
                   return false;
               }
               if (node->next == NULL){ // reached the end of the list without a match
                   // insert a setnode at the head of the list
                   setnode_t *new = setnode_new(key, item);
                   if (new != NULL) {
                       new->next = set->head;
                       set->head = new;
                       return true;
                   }
               }
           }
       }
    }
    return false;
}

/**************** set_find() ****************/
/* see set.h for description */
void *set_find(set_t *set, const char *key) {
    if (set != NULL && key != NULL) {
        // traverse through the linked list
        for (setnode_t *node = set->head; node != NULL; node = node->next) {
            if (strcmp(node->key, key) == 0) {
                // found a match; return the item
                return node->item;
            }
        }
    }
    return NULL;
}

/**************** set_print() ****************/
/* see set.h for description */
void set_print(set_t *set, FILE *fp, 
               void (*itemprint)(FILE *fp, const char *key, void *item)) {
    if (fp != NULL) {
        if (set != NULL) {
            fputc('{', fp);
            // traverse through the linked list
            for (setnode_t *node = set->head; node != NULL; node = node->next) {
                if (itemprint != NULL) {
                    // print each item
                    (*itemprint)(fp, node->key, node->item);
                    if (node->next != NULL){ // separate by commas
                        fputc(',', fp);
                    }
                }
            }
            fputc('}', fp);
        } else {
            fputs("(null)", fp);
        }
    }
}

/**************** set_iterate() ****************/
/* see set.h for description */
void set_iterate(set_t *set, void *arg,
                 void (*itemfunc)(void *arg, const char *key, void *item)) {
    if (set != NULL && itemfunc != NULL) {
        // call itemfunc for every node in the set
        for (setnode_t *node = set->head; node != NULL; node = node->next) {
            (*itemfunc)(arg, node->key, node->item);
        }
    }
}

/**************** set_delete() ****************/
/* see set.h for description */
void set_delete(set_t *set, void (*itemdelete)(void *item)) {
    if (set != NULL) {
        for (setnode_t *node = set->head; node != NULL; ) {
            if (itemdelete != NULL) {
                // caller handles deleting the item in the node
                (*itemdelete)(node->item);
            }
            setnode_t *next = node->next;// store the next node
            count_free(node->key);       // free the key   
            count_free(node);            // free the current node
            node = next;                 // set the current node to the stored next node
        }
        count_free(set);    // free the set
    }
}

/**************** setnode_new() ****************/
/* creates a new setnode for insertion into the set */
static setnode_t *setnode_new(const char *key, void *item) {
    setnode_t *node = count_malloc(sizeof(setnode_t));
    if (node == NULL){
        // error allocating memory for the setnode
        return NULL;
    } else {
        // allocate memory for the key
        node->key = count_malloc(strlen(key)+1);
        if (node->key == NULL){
            // error allocating memory; free the node
            count_free(node);
            return NULL;
        } else {
            strcpy(node->key, key); // copy the key to allow caller to free their key
            node->item = item;      // store the item
            node->next = NULL;
            return node;
        }
    }
}
