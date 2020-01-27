/*
 *  Rhythm_4_4_Loop.c
 *  To use:
 *    if you want to make a module named "foo",
 *    search and replace "4_4_Loop" with Foo (case sensitive)
 *    and "4_4_loop" with "foo" (case sensitive).
 *    Then declare any variables you need in opaque_rhythm_foo_struct,
 *    initalize them in rhythm_foo_new,
 *    free them if necessary in rhythm_foo_destroy.
 *    Musical onsets will be reported in rhythm_4_4_loop_onset
 *      (this is called automatically by the onset tracker)
 *    You should generate one beat of rhythm at a time, in rhythm_4_4_loop_beat
 *      (this is called automatically by the beat tracker at the begining of each beat)
 *  mkrzyzan@asu.edu
 */

#include "Rhythm_Generators.h"

/*--------------------------------------------------------------------*/
void*        rhythm_4_4_loop_destroy (void*);
const char*  rhythm_4_4_loop_name    (void*);
void         rhythm_4_4_loop_onset   (void*, BTT*, unsigned long long);
int          rhythm_4_4_loop_beat    (void*, BTT*, unsigned long long, rhythm_onset_t*, int);

/*--------------------------------------------------------------------*/
const float rhythm_4_4_loop_list[][8] =
{
  { 3, 0.0, 0.5, 0.75},
  { 3, 0.0, 0.25, 0.5},
  { 3, 0.0, 0.25, 0.75},
  { 2, 0.25, 0.5},
};

static const int rhythm_4_4_loop_list_length = sizeof(rhythm_4_4_loop_list) / sizeof(*rhythm_4_4_loop_list);

/*--------------------------------------------------------------------*/
typedef struct opaque_rhythm_4_4_loop_struct
{
  RHYTHM_GENERATOR_SUBCLASS_GUTS ;
  
  /* add instance variables below here */
  int beat_index;
}Rhythm_4_4_Loop;

/*--------------------------------------------------------------------*/
Rhythm* rhythm_4_4_loop_new(BTT* beat_tracker)
{
  Rhythm_4_4_Loop* self = (Rhythm_4_4_Loop*) calloc(1, sizeof(*self));
  
  if(self != NULL)
    {
      self->destroy = rhythm_4_4_loop_destroy;
      self->name    = rhythm_4_4_loop_name;
      self->onset   = rhythm_4_4_loop_onset;
      self->beat    = rhythm_4_4_loop_beat;
    
      /* initalize instance variables below here */
      /* return rhythm_4_4_loop_destroy(self) on failure */
    
    }
  
  return (Rhythm*)self;
}

/*--------------------------------------------------------------------*/
void*      rhythm_4_4_loop_destroy (void* SELF)
{
  Rhythm_4_4_Loop* self = (Rhythm_4_4_Loop*)SELF;
  if(self != NULL)
    {
      /* free any malloc-ed instance variables here */
    
      free(self);
    }
  return (Rhythm*) NULL;
}

/*--------------------------------------------------------------------*/
const char*  rhythm_4_4_loop_name    (void* SELF)
{
  /* just return the name of the module for display */
  static const char* name = "4 4 Loop";
  return name;
}

/*--------------------------------------------------------------------*/
void         rhythm_4_4_loop_onset   (void* SELF, BTT* beat_tracker, unsigned long long sample_time)
{
  Rhythm_4_4_Loop* self = (Rhythm_4_4_Loop*)SELF;
  
}

/*--------------------------------------------------------------------*/
int          rhythm_4_4_loop_beat    (void* SELF, BTT* beat_tracker, unsigned long long sample_time, rhythm_onset_t* returned_rhythm, int returned_rhythm_maxlen)
{
  Rhythm_4_4_Loop* self = (Rhythm_4_4_Loop*)SELF;
  
  int r = self->beat_index;
  ++self->beat_index; self->beat_index %= rhythm_4_4_loop_list_length;
  int len = rhythm_4_4_loop_list[r][0];
  if(len > returned_rhythm_maxlen) len = returned_rhythm_maxlen;
  int i;

  
  for(i=0; i<len; i++)
    {
      returned_rhythm[i].beat_time = rhythm_4_4_loop_list[r][i+1];
      returned_rhythm[i].strength = -1;
      returned_rhythm[i].timbre_class = -1;
    }
  return len;
}

