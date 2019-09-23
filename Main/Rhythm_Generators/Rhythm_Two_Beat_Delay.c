/*
 *  Rhythm_Two_Beat_Delay.c
 *  To use:
 *    if you want to make a module named "foo",
 *    search and replace "Two_Beat_Delay" with Foo (case sensitive)
 *    and "two_beat_delay" with "foo" (case sensitive).
 *    Then declare any variables you need in opaque_rhythm_foo_struct,
 *    initalize them in rhythm_foo_new,
 *    free them if necessary in rhythm_foo_destroy.
 *    Musical onsets will be reported in rhythm_two_beat_delay_onset
 *      (this is called automatically by the onset tracker)
 *    You should generate one beat of rhythm at a time, in rhythm_two_beat_delay_beat
 *      (this is called automatically by the beat tracker at the begining of each beat)
 *  mkrzyzan@asu.edu
 */

#include "Rhythm_Generators.h"
#include <stdlib.h> //calloc

/*--------------------------------------------------------------------*/
void*        rhythm_two_beat_delay_destroy (void*);
const char*  rhythm_two_beat_delay_name    (void*);
void         rhythm_two_beat_delay_onset   (void*, BTT*, unsigned long long);
int          rhythm_two_beat_delay_beat    (void*, BTT*, unsigned long long, float*, int);

/*--------------------------------------------------------------------*/
typedef struct opaque_rhythm_two_beat_delay_struct
{
  RHYTHM_GENERATOR_SUBCLASS_GUTS ;
  
  /* add instance variables below here */
  
}Rhythm_Two_Beat_Delay;

/*--------------------------------------------------------------------*/
Rhythm* rhythm_two_beat_delay_new(BTT* beat_tracker)
{
  Rhythm_Two_Beat_Delay* self = (Rhythm_Two_Beat_Delay*) calloc(1, sizeof(*self));
  
  if(self != NULL)
    {
      self->destroy = rhythm_two_beat_delay_destroy;
      self->name    = rhythm_two_beat_delay_name;
      self->onset   = rhythm_two_beat_delay_onset;
      self->beat    = rhythm_two_beat_delay_beat;
    
      /* initalize instance variables below here */
      /* return rhythm_two_beat_delay_destroy(self) on failure */
    }
  
  return (Rhythm*)self;
}

/*--------------------------------------------------------------------*/
void*      rhythm_two_beat_delay_destroy (void* SELF)
{
  Rhythm_Two_Beat_Delay* self = (Rhythm_Two_Beat_Delay*)SELF;
  if(self != NULL)
    {
      /* free any malloc-ed instance variables here */
    
      free(self);
    }
  return (Rhythm*) NULL;
}

/*--------------------------------------------------------------------*/
const char*  rhythm_two_beat_delay_name    (void* SELF)
{
  /* just return the name of the module for display */
  static const char* name = "Two_Beat_Delay";
  return name;
}

/*--------------------------------------------------------------------*/
void         rhythm_two_beat_delay_onset   (void* SELF, BTT* beat_tracker, unsigned long long sample_time)
{
  Rhythm_Two_Beat_Delay* self = (Rhythm_Two_Beat_Delay*)SELF;
}

/*--------------------------------------------------------------------*/
int          rhythm_two_beat_delay_beat    (void* SELF, BTT* beat_tracker, unsigned long long sample_time, float* returned_rhythm, int returned_rhythm_maxlen)
{
  Rhythm_Two_Beat_Delay* self = (Rhythm_Two_Beat_Delay*)SELF;
  
  returned_rhythm[0] = 0;
  returned_rhythm[1] = 1.0/6.0;
  returned_rhythm[2] = 2.0/6.0;
  returned_rhythm[3] = 3.0/6.0;
  returned_rhythm[4] = 4.0/6.0;
  returned_rhythm[5] = 5.0/6.0;

  
  return 6;
  
  
}

