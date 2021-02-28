/* 
 * hashtable.c - CS50 'hashtable' module
 *
 * see hashtable.h for more information.
 *
 * William Dinauer, Dartmouth CS50 Winter 2021
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "string.h"
#include "set.h"
#include "hashtable.h"
#include "memory.h"
#include "jhash.h"

typedef struct hashtable {
    long size;
    set_t **array;
} hashtable_t;

/**************** hashtable_new() ****************/
/* see hashtable.h for description */
hashtable_t *hashtable_new(const int num_slots) {
    hashtable_t *ht = count_malloc(sizeof(hashtable_t));
    if (ht == NULL) {
        // error allocating memory for ht
        return NULL;
    } else {
        ht->size = 0; // initialize the size
        ht->array = count_calloc(num_slots, sizeof(set_t*));
        if (ht->array == NULL) {
            // error allocating memory for the array
            count_free(ht);
            return NULL;
        } else {
            int i;
            for (i = 0; i < num_slots; i++) {
                // create a set for each slot in the array
                ht->array[i] = set_new();
                if (ht->array[i] == NULL) {
                    // error allocating memory for set; delete all previously created sets
                    int x;
                    for (x = 0; x < ht->size; x++) {
                        count_free(ht->array[x]);
                    }
                    count_free(ht->array);
                    count_free(ht);
                    return NULL;
                } else {
                    ht->size++;
                }
            }
            return ht;
        }
    }
    return NULL;
}

/**************** hashtable_insert() ****************/
/* see hashtable.h for description */
bool hashtable_insert(hashtable_t *ht, const char *key, void *item)
{
   if (ht != NULL && key != NULL && item != NULL) {
      // call the hash function for the key, placing into the according slot
      long slot = JenkinsHash(key, ht->size); 
      if (set_insert(ht->array[slot], key, item)) {
          return true;
      } else {
          // insert failed; key may already be in the set
          return false;
      }
   }
   return false;
}

/**************** hashtable_find() ****************/
/* see hashtable.h for description */
void *hashtable_find(hashtable_t *ht, const char *key)
{
    if (ht != NULL && key != NULL) {
        // search for the key in the slot it hashes to
        long slot = JenkinsHash(key, ht->size);
        return set_find(ht->array[slot], key);
    }
    return NULL;
}

/**************** hashtable_print() ****************/
/* see hashtable.h for description */
void hashtable_print(hashtable_t *ht, FILE *fp, 
                     void (*itemprint)(FILE *fp, const char *key, void *item)) 
{
    if (fp != NULL) {
        if (ht != NULL) {
            int x;
            // iterate over all slots/sets
            for (x = 0; x < ht->size; x++) {
                fprintf(fp, "slot %d: ", x);
                if (itemprint != NULL){
                    //print the set
                    set_print(ht->array[x], fp, itemprint);
                }
                fputc('\n', fp);
            }
        } else {
            fputs("(null)", fp);
        }
    }
}

/**************** hashtable_iterate() ****************/
/* see hashtable.h for description */
void hashtable_iterate(hashtable_t *ht, void *arg,
                       void (*itemfunc)(void *arg, const char *key, void *item))
{
    if (ht != NULL && itemfunc != NULL) {
        int x;
        // iterate over all slots/sets
        for (x = 0; x < ht->size; x++) {
            set_iterate(ht->array[x], arg, itemfunc);
        }
    }
}

/**************** hashtable_delete() ****************/
/* see hashtable.h for description */
void hashtable_delete(hashtable_t *ht, void (*itemdelete)(void *item))
{
    if (ht != NULL) {
        int x;
        // free every set
        for (x = 0; x < ht->size; x++) {
            set_delete(ht->array[x], itemdelete);
        }
        count_free(ht->array); // free the array
        count_free(ht);        // free the hashtable
    }
}
