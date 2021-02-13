/*!
 * @author
 *  Written by Michael Krzyzaniak at Arizona State 
 *  University's School of Arts, Media + Engineering
 *  and School of Film, Dance and Theatre
 *  in Summer of 2015.
 *
 *  mkrzyzan@asu.edu
 *
 * @unsorted
 */
 
#ifndef __HASHTABLE__
#define __HASHTABLE__ 1

#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

#include "List.h"
#include "stdint.h"
#include "stdlib.h"

typedef struct  opaque_hashtable_struct  Hashtable;
typedef struct  opaque_hash_entry_struct hash_entry_t;

typedef char*                   hash_key_t;
typedef list_data_t             hash_value_t;
typedef list_data_deallocator_t hash_value_deallocator_t;

Hashtable*   hash_new                     (int    capacity);
Hashtable*   hash_destroy                 (Hashtable* self, BOOL should_deallocate);
Hashtable*   hash_copy                    (Hashtable* self); //shallow-copies values

void         hash_store_value_for_key     (Hashtable* self, hash_key_t key, hash_value_t value, hash_value_deallocator_t value_deallocator);
hash_value_t hash_get_value_for_key       (Hashtable* self, hash_key_t key);
List*        hash_get_values_for_key      (Hashtable* self, hash_key_t key); //user should destroy List

/*does not deallocate the key or the value.*/
hash_value_t hash_remove_value_for_key    (Hashtable* self, hash_key_t key, BOOL should_deallocate);
int          hash_count                   (Hashtable* self);
int          hash_get_all_keys            (Hashtable* self, char* returned_keys[]);
List*        hash_get_all_entries         (Hashtable* self); //user should destroy List
hash_key_t   hash_entry_get_key           (hash_entry_t* self);
hash_value_t hash_entry_get_value         (hash_entry_t* self);

/*you dont need to call this directly but other modules can use it*/
uint32_t     hash_hash(char* str);
void         hash_print_collision_data    (Hashtable* self);

#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif   // __HASHTABLE__
