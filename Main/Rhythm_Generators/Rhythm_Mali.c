/*
 *  Rhythm_Mali.c
 *  To use:
 *    if you want to make a module named "foo",
 *    search and replace "4_4_Loop" with Foo (case sensitive)
 *    and "4_4_loop" with "foo" (case sensitive).
 *    Then declare any variables you need in opaque_rhythm_foo_struct,
 *    initalize them in rhythm_foo_new,
 *    free them if necessary in rhythm_foo_destroy.
 *    Musical onsets will be reported in rhythm_mali_onset
 *      (this is called automatically by the onset tracker)
 *    You should generate one beat of rhythm at a time, in rhythm_mali_beat
 *      (this is called automatically by the beat tracker at the begining of each beat)
 *  mkrzyzan@asu.edu
 */

#include "Rhythm_Generators.h"

/*--------------------------------------------------------------------*/
void*        rhythm_mali_destroy (void*);
const char*  rhythm_mali_name    (void*);
void         rhythm_mali_onset   (void*, BTT*, unsigned long long);
int          rhythm_mali_beat    (void*, BTT*, unsigned long long, rhythm_onset_t*, int);

/*--------------------------------------------------------------------*/
const float rhythm_mali_list[][8] =
{
  { 3, 0, 0.0/3.0, 2.0/3.0},
  { 3, 0, 1.0/3.0, 2.0/3.0},
  { 2, 0, 1.0/3.0         },
  { 3, 0, 0.0/3.0, 2.0/3.0},

  { 3, 0, 0.0/3.0, 2.0/3.0},
  { 2, 0, 1.0/3.0         },
  { 3, 0, 0.0/3.0, 1.0/3.0},
  { 3, 0, 0.0/3.0, 2.0/3.0},
};

/*--------------------------------------------------------------------*/
/* you better make sure it matches mali_list in size */
const int rhythm_mali_solenoid_list[][8] =
{
  { 3, 5, 0, 1},
  { 3, 5, 1, 1},
  { 2, 5, 1   },
  { 3, 5, 1, 1},

  { 3, 4, 0, 1},
  { 2, 4, 1, 1},
  { 3, 4, 1   },
  { 3, 4, 1, 1},
};

static const int rhythm_mali_list_length = sizeof(rhythm_mali_list) / sizeof(*rhythm_mali_list);

/*--------------------------------------------------------------------*/
typedef struct opaque_rhythm_c_struct
{
  RHYTHM_GENERATOR_SUBCLASS_GUTS ;
  
  /* add instance variables below here */
  int beat_index;
}Rhythm_Mali;

/*--------------------------------------------------------------------*/
Rhythm* rhythm_mali_new(BTT* beat_tracker)
{
  Rhythm_Mali* self = (Rhythm_Mali*) calloc(1, sizeof(*self));
  
  if(self != NULL)
    {
      self->destroy = rhythm_mali_destroy;
      self->name    = rhythm_mali_name;
      self->onset   = rhythm_mali_onset;
      self->beat    = rhythm_mali_beat;
    
      /* initalize instance variables below here */
      /* return rhythm_mali_destroy(self) on failure */
    
    }
  
  return (Rhythm*)self;
}

/*--------------------------------------------------------------------*/
void*      rhythm_mali_destroy (void* SELF)
{
  Rhythm_Mali* self = (Rhythm_Mali*)SELF;
  if(self != NULL)
    {
      /* free any malloc-ed instance variables here */
    
      free(self);
    }
  return (Rhythm*) NULL;
}

/*--------------------------------------------------------------------*/
const char*  rhythm_mali_name    (void* SELF)
{
  /* just return the name of the module for display */
  static const char* name = "Mali";
  return name;
}

/*--------------------------------------------------------------------*/
void         rhythm_mali_onset   (void* SELF, BTT* beat_tracker, unsigned long long sample_time)
{
  Rhythm_Mali* self = (Rhythm_Mali*)SELF;
  
}

/*--------------------------------------------------------------------*/
int          rhythm_mali_beat    (void* SELF, BTT* beat_tracker, unsigned long long sample_time, rhythm_onset_t* returned_rhythm, int returned_rhythm_maxlen)
{
  Rhythm_Mali* self = (Rhythm_Mali*)SELF;
  
  int r = self->beat_index;
  ++self->beat_index; self->beat_index %= rhythm_mali_list_length;
  int len = rhythm_mali_list[r][0];
  if(len > returned_rhythm_maxlen) len = returned_rhythm_maxlen;
  int i;

  
  for(i=0; i<len; i++)
    {
      returned_rhythm[i].beat_time = rhythm_mali_list[r][i+1];
      returned_rhythm[i].strength = -1;
      returned_rhythm[i].timbre_class = rhythm_mali_solenoid_list[r][i+1];
    }
  return len;
}

