#include "List.h"
#include <stdio.h>

typedef struct
{
  int asdf;
  float poiu;
}Object;

Object* object_new()
{
  Object* self = calloc(1, sizeof(*self));
  return self;
}

Object* object_destroy(Object* self)
{
  if(self != NULL)
    {
      free(self);
    }
  return (Object*) NULL;
}

int main(void)
{
  List* list = list_new();
  if(list == NULL) {perror("unable to create list"); exit(-1);}

  Object* object_1 = object_new();
  if(object_1 == NULL) {perror("unable to create object 1"); exit(-1);}

  Object* object_2 = object_new();
  if(object_2 == NULL) {perror("unable to create object 2"); exit(-1);}

  Object* object_3 = object_new();
  if(object_3 == NULL) {perror("unable to create object 3"); exit(-1);}
  list_append_data(list, object_1, (list_data_deallocator_t)object_destroy);
  list_append_data(list, object_2, (list_data_deallocator_t)object_destroy);
  list_append_data(list, object_3, (list_data_deallocator_t)object_destroy);
  
  Object* o;
  list_iterator_t iter;
  int i = 0;
  while((o = (Object*)list_advance_iterator(list, &iter)) != NULL)
     {
       if(i == 0)
         list_remove_data(list, o, YES);
      ++i;
     }
  
  list_destroy(list, YES);
  
  exit(0);
}
