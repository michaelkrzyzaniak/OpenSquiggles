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

#include "Hashtable.h"
#include <stdio.h> //for debugging only
#include <string.h>



struct opaque_hash_entry_struct
{
  hash_key_t   key;
  hash_value_t value;
  hash_value_deallocator_t value_deallocator;
};

hash_entry_t* hash_destroy_entry(hash_entry_t* entry);

/*-------------------------------------------------------------------------------------*/
hash_entry_t* hash_new_entry(hash_key_t key, hash_value_t value, hash_value_deallocator_t value_deallocator)
{
  hash_entry_t* entry = malloc(sizeof(*entry));
  if(entry != NULL)
    {
      int len = strlen(key);
      entry->key = calloc(len, sizeof(*entry->key)+1);
      if(entry->key == NULL)
        return hash_destroy_entry(entry);
      memcpy(entry->key, key, len+1);
      entry->value = value;
      entry->value_deallocator = value_deallocator;
    }
  return entry;
}

/*-------------------------------------------------------------------------------------*/
//entry->value will already have been destroyed if it was supposed to be
hash_entry_t* hash_destroy_entry(hash_entry_t* entry)
{
  if(entry != NULL)
    {
      if(entry->key != NULL)
        free(entry->key);
      free(entry);
    }
  return (hash_entry_t*)NULL;
}

/*-------------------------------------------------------------------------------------*/
struct opaque_hashtable_struct
{
  List** entries;
  int capacity;
  int count;
};

/*-------------------------------------------------------------------------------------*/
Hashtable* hash_new(int capacity)
{
  Hashtable* self = (Hashtable*) calloc(sizeof(*self), 1);
  if(self != NULL)
    {
      self->capacity = capacity;
      self->entries = (List**) calloc(capacity, sizeof(List*));
      if(self->entries == NULL) self = hash_destroy(self, NO);
    }
  return self;
}

/*-------------------------------------------------------------------------------------*/
Hashtable*   hash_copy                    (Hashtable* self)
{
  Hashtable* copy = hash_new(self->capacity);
  if(copy != NULL)
    {
      List* l = hash_get_all_entries(self);
      if(l != NULL)
        {
          list_iterator_t i = list_reset_iterator(l);
          hash_entry_t* entry;
          while((entry = list_advance_iterator(l, &i)) != NULL)
            hash_store_value_for_key(self, entry->key, entry->value, entry->value_deallocator);
          list_destroy(l, NO);
        }
    }
  return copy;
}

/*-------------------------------------------------------------------------------------*/
Hashtable* hash_destroy(Hashtable* self, BOOL should_deallocate)
{
  if(self != NULL)
    {
      if(self->entries != NULL)
        {
          int i;
          for(i=0; i<self->capacity; i++)
            if(self->entries[i] != NULL)
              list_destroy(self->entries[i], should_deallocate);
          free(self->entries);
        }
        
      free(self);
    }
  return (Hashtable*)NULL;
}

/*-------------------------------------------------------------------------------------*/
void hash_store_value_for_key (Hashtable* self, hash_key_t key, hash_value_t value, hash_value_deallocator_t value_deallocator)
{
  uint32_t h = hash_hash(key) % self->capacity;
  if(self->entries[h] == NULL)
    {
      self->entries[h] = list_new();
      if(self->entries[h] == NULL) return; //error, could not create list
    }
    
  hash_entry_t* entry = hash_new_entry(key, value, value_deallocator);
  if(entry == NULL) return; //error, could not create entry
  
  list_append_data(self->entries[h], entry, (list_data_deallocator_t)hash_destroy_entry);
  self->count++;
}

/*-------------------------------------------------------------------------------------*/
hash_value_t hash_get_value_for_key (Hashtable* self, hash_key_t key)
{
  uint32_t h = hash_hash(key) % self->capacity;
  List* list = self->entries[h];
  hash_entry_t* entry = NULL;
  hash_value_t* result = NULL;
  
  if(list != NULL)
    {
      list_iterator_t i = list_reset_iterator(list);
      while((entry = list_advance_iterator(list, &i)) != NULL)
        {
          if(strcmp(key, entry->key) == 0)
            {
              result = entry->value;
              break;
            }            
        }
    }

  return result;
}

/*-------------------------------------------------------------------------------------*/
List*        hash_get_values_for_key      (Hashtable* self, hash_key_t key)
{
  List* result = NULL;
  uint32_t h = hash_hash(key) % self->capacity;
  List* list = self->entries[h];
  
  if(list != NULL)
    {
      result = list_new();
      if(result != NULL)
        {
          hash_entry_t* entry;
          list_iterator_t i = list_reset_iterator(list);
          while((entry = list_advance_iterator(list, &i)) != NULL)
            if(strcmp(key, entry->key) == 0)
              list_append_data(result, entry->value, NULL);
        }
    }
  return result;
}

/*-------------------------------------------------------------------------------------*/
hash_value_t hash_remove_value_for_key (Hashtable* self, hash_key_t key, BOOL should_destroy)
{
  //remove all values matching key
  uint32_t h = hash_hash(key) % self->capacity;
  hash_value_t value = NULL;
  List* list = self->entries[h];
  hash_entry_t* entry;
  
  if(list != NULL)
    {
      list_iterator_t i = list_reset_iterator(list);
      while((entry = list_advance_iterator(list, &i)) != NULL)
        if(strcmp(key, entry->key) == 0)
          {
            value = entry->value;
            if(should_destroy)
              value = entry->value_deallocator(value);
            list_remove_data(list, entry, YES);
            --self->count;
          }
    }

  return value;
}

/*-------------------------------------------------------------------------------------*/
int         hash_count                 (Hashtable* self)
{
  return self->count;
}

/*-------------------------------------------------------------------------------------*/
int        hash_get_all_keys               (Hashtable* self, char* returned_keys[])
{
  int i, n=0;
  hash_entry_t* entry;
  List* list;
  
  for(i=0; i<self->capacity; i++)
    {
      list = self->entries[i];
      if(list != NULL)
        {
          list_iterator_t iter;
          iter = list_reset_iterator(list);
          while((entry = list_advance_iterator(list, &iter)) != NULL)
            {
              *returned_keys++ = entry->key;
              n++;
            }
        }
    } 
  return n;
}

/*-------------------------------------------------------------------------------------*/
List*       hash_get_all_entries         (Hashtable* self)
{
  List* result = list_new();
  List* l;
  hash_entry_t* entry;
  
  if(result != NULL)
    {
      int i;
      for(i=0; i<self->capacity; i++)
        {
          l = self->entries[i];
          if(l != NULL)
            {
              list_iterator_t i = list_reset_iterator(l);
              while((entry = list_advance_iterator(l, &i)) != NULL)
                list_append_data(result, entry, NULL);
            }
        }
    }
  return result;
}

/*-------------------------------------------------------------------------------------*/
hash_key_t   hash_entry_get_key           (hash_entry_t* self)
{
  return self->key;
}

/*-------------------------------------------------------------------------------------*/
hash_value_t hash_entry_get_value         (hash_entry_t* self)
{
  return self->value;
}

/*-------------------------------------------------------------------------------------*/
void hash_print_collision_data(Hashtable* self)
{
  int i, capacity = self->capacity;
  int num_filled_entries = 0;
  int num_vacant_entries = 0;
  int num_collisions    = 0;
  
  for(i=0; i<capacity; i++)
    {
      if(self->entries[i] == NULL)
        num_vacant_entries++;
      else
        {
          num_filled_entries++;
          num_collisions += list_count(self->entries[i]) - 1;
        }
    }
  printf("capacity: %i, num_filled_entries: %i, num_vacant_entries: %i, num_collisions: %i\n", capacity, num_filled_entries, num_vacant_entries, num_collisions);
}

/*-------------------------------------------------------------------------------------*/
uint32_t hash_hash(char* str)
{
	uint32_t hash = 5381;
	while (*str != '\0')
		hash = hash * 33 ^ (unsigned int)*str++;
	return hash;
}