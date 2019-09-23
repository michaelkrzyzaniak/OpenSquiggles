/*_--__-_--___-----_-------_------------_-_-----_-----_----------------------------------
|  \/  | |/ / |	  (_)_ __ | | _____  __| | |   (_)___| |_   ___	
| |\/| | ' /| |	  | | '_ \| |/ / _ \/ _` | |   | / __| __| / __|
| |  | | . \| |___| | |	| |   <	 __/ (_| | |___| \__ \ |_ | (__	
|_|  |_|_|\_\_____|_|_|	|_|_|\_\___|\__,_|_____|_|___/\__(_)___|

---------------------------------------------------------------------------------------*/
  
#include "List.h"
  
/*--------_----------_------------_--------_---------------_---_-------------------------
 _ __ _	_(_)_ ____ _| |_ ___   __| |___	__| |__	_ _ _ __ _| |_(_)___ _ _  ___
| '_ \ '_| \ V / _` |  _/ -_) /	_` / -_) _| / _` | '_/ _` |  _|	/ _ \ '	\(_-<
| .__/_| |_|\_/\__,_|\__\___| \__,_\___\__|_\__,_|_| \__,_|\__|_\___/_||_/__/
|_|    	       	       	       	       	       	       	       	       	     
---------------------------------------------------------------------------------------*/

typedef struct
{ 
  void*       prev;
  list_data_t data;
  void*       next;
  list_data_deallocator_t deallocate_data;
}list_entry_t;
  
/*-------------------------------------------------------------------------------------*/
struct opaque_list_struct
{ 
  unsigned count;
  list_entry_t         *first_entry;
  list_entry_t         *last_entry;
};

/*-------------------------------------------------------------------------------------*/
list_entry_t*      list_new_entry                ();
list_entry_t*      list_entry_at_index           (List* self, unsigned index);
void               list_remove_entry_at_index    (List* self, unsigned index, BOOL should_deallocate);
void               list_clean_entry              (List* self, list_entry_t* entry);

/*------------_---_----------------------------------------------------------------------
 _ _ ___ _  _| |_(_)_ _	 ___ ___
| '_/ _	\ || | 	_| | ' \/ -_|_-<
|_| \___/\_,_|\__|_|_||_\___/__/
---------------------------------------------------------------------------------------*/

list_entry_t* list_new_entry()
{
  return (list_entry_t*)malloc(sizeof(list_entry_t));
}      

/*-------------------------------------------------------------------------------------*/
list_entry_t* list_entry_at_index(List* self, unsigned index)
{
  list_entry_t* this_entry = self->first_entry;
  
  long max_index = list_count(self)-1;
  if(index > max_index) return NULL;
  
  while(index-- > 0)
    this_entry = this_entry->next;

  return this_entry;
}

/*-------------------------------------------------------------------------------------*/
list_entry_t* list_entry_with_data(List* self, list_data_t data)
{
  list_entry_t* this_entry = self->first_entry;
  
  while(this_entry != NULL)
    {
      if(this_entry->data == data)
        break;
      this_entry = this_entry->next;
    }
    
  return this_entry;
}


/*-------------------------------------------------------------------------------------*/
void list_clean_entry(List* self, list_entry_t* entry)
{
  void* data = entry->data;
  if(data != NULL)
    {
      if(entry->deallocate_data != NULL)
        entry->data = entry->deallocate_data(entry->data);
      while(list_contains_data(self, data))
        list_replace_data(self, data, NULL, NO);
    }
}

/*-------------------------------------------------------------------------------------*/
void list_remove_entry(List* self, list_entry_t* entry, BOOL should_deallocate)
{
  if(entry != NULL)
    {
      if (should_deallocate) list_clean_entry(self, entry);
      if(entry->prev != NULL)
        ((list_entry_t*)entry->prev)->next = entry->next;
      else
        self->first_entry = (list_entry_t*)entry->next;
      if(entry->next != NULL)
        ((list_entry_t*)entry->next)->prev = entry->prev;
      else self->last_entry = entry->prev;
      free(entry);
      self->count--;
    }
}

/*-------------------------------------------------------------------------------------*/
List* list_new()
{
  List* self = calloc(1, sizeof(*self));

  return self;
}

/*-------------------------------------------------------------------------------------*/
List* list_shallow_copy(List* self)
{
  List* copy = list_new();
  
  if(copy != NULL)
    {
      list_entry_t* current = self->first_entry;
      while(current != NULL)
        {
          list_append_data(copy, current->data, current->deallocate_data);
          current = current->next;
        }
    }
  return copy;
}

/*-------------------------------------------------------------------------------------*/
unsigned list_count(List* self)
{
  return self->count;
}

/*-------------------------------------------------------------------------------------*/
BOOL          list_contains_data      (List* self, list_data_t data)
{
  list_entry_t* e = list_entry_with_data(self, data);
  return (e == NULL) ? NO : YES;
}

/*-------------------------------------------------------------------------------------*/
void list_append_data(List* self, list_data_t data, list_data_deallocator_t deallocator)
{
  list_entry_t* this_entry = list_new_entry();
  if(this_entry != NULL)
    {
      if(self->first_entry == NULL) self->first_entry = this_entry; 
      list_entry_t* prev_entry = self->last_entry;
      this_entry->prev = prev_entry;
      if(prev_entry != NULL)prev_entry->next = this_entry;
      self->last_entry = this_entry;
    
      this_entry->next = NULL;
      this_entry->data = data;
      this_entry->deallocate_data = deallocator;
      self->count++;
    }
}

/*-------------------------------------------------------------------------------------*/
void list_insert_data_at_index(List* self, list_data_t data, unsigned index, list_data_deallocator_t deallocator)
{
  if(index >= list_count(self)) 
    list_append_data(self, data, deallocator);
  else
    {
      list_entry_t* this_entry = list_new_entry();
      if(this_entry != NULL)
        {
          list_entry_t* nextEntry = list_entry_at_index(self, index);
          list_entry_t* prev_entry = nextEntry->prev;
          
          this_entry->next     = nextEntry;
          this_entry->prev = prev_entry;
          
          if(prev_entry != NULL)
            prev_entry->next = this_entry;
          if(nextEntry != NULL)
	          nextEntry->prev = this_entry;
          if(index == 0) 
	          self->first_entry = this_entry;
            
          this_entry->data = data;
          this_entry->deallocate_data = deallocator;
          self->count++;
        }
    }
}

/*-------------------------------------------------------------------------------------*/
void list_remove_data_at_index(List* self, unsigned index, BOOL should_deallocate)
{
  list_entry_t* e = list_entry_at_index(self, index);
  list_remove_entry(self, e, should_deallocate);
}

/*-------------------------------------------------------------------------------------*/
void list_remove_data (List* self, list_data_t data, BOOL should_deallocate)
{
  list_entry_t* e = list_entry_with_data(self, data);
  list_remove_entry(self, e, should_deallocate);  
}

/*-------------------------------------------------------------------------------------*/
list_data_t list_data_at_index(List* self, unsigned index)
{
  return list_entry_at_index(self, index)->data;
}

/*-------------------------------------------------------------------------------------*/
void list_replace_data_at_index(List* self, list_data_t data, unsigned index, BOOL should_deallocate)
{
  list_entry_t* this_entry = list_entry_at_index(self, index);
  if(this_entry != NULL)
    {
      if(should_deallocate)
        list_clean_entry(self, this_entry);
      this_entry->data = data;
    }
}

/*-------------------------------------------------------------------------------------*/
void          list_replace_data (List* self, list_data_t old_data, list_data_t new_data,  BOOL should_deallocate)
{
  list_entry_t* this_entry = list_entry_with_data(self, old_data);
  if(this_entry != NULL)
    {
      if(should_deallocate)
        list_clean_entry(self, this_entry);
      this_entry->data = new_data;
    }
}

/*-------------------------------------------------------------------------------------*/
list_iterator_t list_reset_iterator      (List* self)
{
  return (list_iterator_t) self->first_entry;
}

/*-------------------------------------------------------------------------------------*/
list_data_t list_advance_iterator    (List* self, list_iterator_t* iterator)
{
  list_data_t result = NULL;
  if(*iterator != NULL)
    {
      result = ((list_entry_t*)(*iterator))->data;
      *iterator = (list_iterator_t)(((list_entry_t*)(*iterator))->next);
    }
  return result;
}

/*-------------------------------------------------------------------------------------*/
void list_clear(List* self, BOOL should_deallocate)
{
  while(list_count(self))
    list_remove_entry(self, self->first_entry, should_deallocate);
}

/*-------------------------------------------------------------------------------------*/
void list_swap_entry_content(list_entry_t* a, list_entry_t* b)
{
  list_data_t*            temp_data    = a->data;
  list_data_deallocator_t temp_dealloc = a->deallocate_data;
  a->data            = b->data;
  a->deallocate_data = b->deallocate_data;
  b->data            = temp_data;
  b->deallocate_data = temp_dealloc;
}

/*-------------------------------------------------------------------------------------*/
unsigned list_partition(List* self, unsigned low_index, unsigned high_index, list_sort_callback_t sort_callback, BOOL ascending, void* user_data)
{
  unsigned i, result=low_index;
  unsigned pivot_index = low_index + ((high_index - low_index) / 2);
  
  list_entry_t* pivot_entry   = list_entry_at_index(self, pivot_index);
  list_entry_t* high_entry    = list_entry_at_index(self, high_index );
  list_entry_t* store_entry   = list_entry_at_index(self, low_index);
  list_entry_t* current_entry = store_entry;
  
  list_swap_entry_content(pivot_entry, high_entry);
  
  for(i=low_index; i<high_index; i++)
    {
      if(sort_callback(high_entry->data, current_entry->data, ascending, user_data))
        {
          list_swap_entry_content(current_entry, store_entry);
          store_entry = store_entry->next;
          result++;
        }
      current_entry = current_entry->next;
    }
  
  list_swap_entry_content(store_entry, high_entry);
  
  return result;
}

/*-------------------------------------------------------------------------------------*/
void list_quicksort(List* self, unsigned low_index, unsigned high_index, list_sort_callback_t sort_callback, BOOL ascending, void* user_data)
{
  if(low_index < high_index)
    {
      unsigned p = list_partition(self, low_index, high_index, sort_callback, ascending, user_data);
      
      if(p > 0) list_quicksort(self, low_index, p-1, sort_callback, ascending, user_data);
      list_quicksort(self, p+1, high_index, sort_callback, ascending, user_data);
    }
}

/*-------------------------------------------------------------------------------------*/
void list_sort                        (List* self, list_sort_callback_t sort_callback, BOOL ascending, void* user_data)
{
  if(list_count(self) < 2) return;
  list_quicksort(self, 0, self->count-1, sort_callback, ascending, user_data);
}

/*-------------------------------------------------------------------------------------*/
void list_sort_range                   (List* self, unsigned startIndex, unsigned range, list_sort_callback_t sort_callback, BOOL ascending, void* user_data)
{
  if(list_count(self) < 2) return;
  if(range <= 1) return;
  
  unsigned endIndex = startIndex + range - 1;
  if(endIndex >= self->count) endIndex = self->count - 1;
  //quicksort will catch if startIndex > endIndex or startIndex > self->count
  list_quicksort(self, startIndex, endIndex, sort_callback, ascending, user_data);
}
/*-------------------------------------------------------------------------------------*/
List* list_destroy(List* self, BOOL should_deallocate)
{
  if(self != NULL)
    {
      list_clear(self, should_deallocate);
      free(self);
    }
  return (List*) NULL;
}

