/*_--__-_--___-----_-------_------------_-_-----_-----_-----_----------------------------
|  \/  | |/ / |	  (_)_ __ | | _____  __| | |   (_)___| |_  | |__  
| |\/| | ' /| |	  | | '_ \| |/ / _ \/ _` | |   | / __| __| | '_	\ 
| |  | | . \| |___| | |	| |   <	 __/ (_| | |___| \__ \ |_ _| | | |
|_|  |_|_|\_\_____|_|_|	|_|_|\_\___|\__,_|_____|_|___/\__(_)_| |_|

---------------------------------------------------------------------------------------*/
/*
  Sept 19 2019 Fixed so that when the same object is in the list multiple times
               it is not deallocated multiple times, causing crash. 
*/
#ifndef __MK_LINKED_LIST__
#define __MK_LINKED_LIST__
  
/*---------_---------_-------------------------------------------------------------------
(_)_ _ 	__| |_ 	_ __| |___ ___
| | ' \/ _| | || / _` /	-_|_-<
|_|_||_\__|_|\_,_\__,_\___/__/
---------------------------------------------------------------------------------------*/
  
#include <stdlib.h>

#ifndef BOOL
#define BOOL int  

#define NO  0  
#define YES (!NO)
#endif

/*---------------------------------------------------------------------------------------
| |_ _ 	_ _ __ 	___ ___
|  _| || | '_ \/ -_|_-<
 \__|\_, | .__/\___/__/
     |__/|_|   	       
---------------------------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

//---------------------------------------------------------------------------------------
typedef void* list_data_t;
typedef void* (*list_data_deallocator_t)   (list_data_t);
typedef BOOL  (*list_sort_callback_t)      (list_data_t a, list_data_t b, BOOL ascending, void* user_data);
typedef void* list_iterator_t;

//---------------------------------------------------------------------------------------
typedef struct opaque_list_struct List;
  
  
  
/*------------_-------_------------------------------------------------------------------
 _ __ _	_ ___| |_ ___| |_ _  _ _ __  ___ ___
| '_ \ '_/ _ \ 	_/ _ \ 	_| || |	'_ \/ -_|_-<
| .__/_| \___/\__\___/\__|\_, |	.__/\___/__/
|_|    	       	       	  |__/|_|      	    
---------------------------------------------------------------------------------------*/
  
  
  
//---------------------------------------------------------------------------------------
/*  create an empty list                                                               */

List*         list_new                 ();


//---------------------------------------------------------------------------------------
/*  copy list but don't copy objects in list                                           */

List*         list_shallow_copy        (List* self);

//---------------------------------------------------------------------------------------
/*  returns the number of entries in the list                                          */

unsigned      list_count               (List* self);

//---------------------------------------------------------------------------------------
/*  YES if list currently contains the data                                             */

BOOL          list_contains_data       (List* self, list_data_t data);

//---------------------------------------------------------------------------------------
/*  appends data to the end of list                                                    */

void          list_append_data         (List* self, list_data_t data, list_data_deallocator_t deallocator);


//---------------------------------------------------------------------------------------
/*  puts data into list at index                                                       */

void          list_insert_data_at_index(List* self, list_data_t data, unsigned index, list_data_deallocator_t deallocator);


//---------------------------------------------------------------------------------------
/*  remove the data at the specified index                                             */

void          list_remove_data_at_index(List* self, unsigned index, BOOL should_deallocate);

//---------------------------------------------------------------------------------------
/*  remove the specified data                                                          */

void          list_remove_data         (List* self, list_data_t data, BOOL should_deallocate);

//---------------------------------------------------------------------------------------
/*  returns the data in the list at the given index                                    */

list_data_t    list_data_at_index      (List* self, unsigned index);

//---------------------------------------------------------------------------------------
/*  replaces the data in the list at the given index with data                         */

void          list_replace_data_at_index (List* self, list_data_t data, unsigned index,  BOOL should_deallocate);

//---------------------------------------------------------------------------------------
/*  replaces the data in the list with data                                            */

void          list_replace_data          (List* self, list_data_t old_data, list_data_t new_data,  BOOL should_deallocate);

//---------------------------------------------------------------------------------------
/*  removes and destroys all of the entrise in the list                                */

void          list_clear              (List* self,  BOOL should_deallocate);


//---------------------------------------------------------------------------------------
/*  clears the list, and frees it and all of its resources                             */

List*         list_destroy            (List* self, BOOL should_deallocate);

//---------------------------------------------------------------------------------------
/*  resets the iterator. returns NULL if the list is empty, otherwise returns some     */
/*  opaque struct this module uses for iterating. see listAdvanceIterator() for        */
/*  a discussion of proper use.                                                        */

list_iterator_t list_reset_iterator      (List* self); 


//---------------------------------------------------------------------------------------
/*  advance the iterator. returns the next data in the list until there is no more     */
/*  data, then returns NULL. correct usage is as follows:                              */
/*                                                                                     */
/*  list_iterator_t i;                                                                 */
/*  list_data_t current_data;                                                          */
/*  i = list_reset_iterator(list);                                                     */
/*  while((current_data = list_advance_iterator(list, &i)) != NULL)                     */
/*    {                                                                                */
/*      //do something to current_data                                                 */
/*    }                                                                                */
/*                                                                                     */
/*                                                                                     */
/*  This is much, much more efficient than calling listDataAtIndex() for each          */
/*  entry in the list. It is now safe to remove current_data while iterating.          */

list_data_t   list_advance_iterator    (List* self, list_iterator_t* iterator); 


//---------------------------------------------------------------------------------------
/*  Quicksort the list. sortCallback should return true if (a > b) to sort in ascending*/
/*  order, or false for descending                                                     */

void          list_sort                (List* self, list_sort_callback_t sort_callback, BOOL ascending, void* user_data);
void          list_sort_range          (List* self, unsigned start_index, unsigned range, list_sort_callback_t sort_callback, BOOL ascending, void* user_data);

//---------------------------------------------------------------------------------------
#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif   //MK_LINKED_LIST

