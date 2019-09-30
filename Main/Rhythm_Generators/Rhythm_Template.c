/*
 *  Rhythm_Template.c
 *  To use:
 *    if you want to make a module named "foo",
 *    search and replace "Template" with Foo (case sensitive)
 *    and "template" with "foo" (case sensitive).
 *    Then declare any variables you need in opaque_rhythm_foo_struct,
 *    initalize them in rhythm_foo_new,
 *    free them if necessary in rhythm_foo_destroy.
 *    Musical onsets will be reported in rhythm_template_onset
 *      (this is called automatically by the onset tracker)
 *    You should generate one beat of rhythm at a time, in rhythm_template_beat
 *      (this is called automatically by the beat tracker at the begining of each beat)
 *  mkrzyzan@asu.edu
 */

#include "Rhythm_Generators.h"
#include <stdlib.h> //calloc

/*--------------------------------------------------------------------*/
void*        rhythm_template_destroy (void*);
const char*  rhythm_template_name    (void*);
void         rhythm_template_onset   (void*, BTT*, unsigned long long);
int          rhythm_template_beat    (void*, BTT*, unsigned long long, rhythm_onset_t*, int);

/*--------------------------------------------------------------------*/
typedef struct opaque_rhythm_template_struct
{
  RHYTHM_GENERATOR_SUBCLASS_GUTS ;
  
  /* add instance variables below here */
  
}Rhythm_Template;

/*--------------------------------------------------------------------*/
Rhythm* rhythm_template_new(BTT* beat_tracker)
{
  Rhythm_Template* self = (Rhythm_Template*) calloc(1, sizeof(*self));
  
  if(self != NULL)
    {
      self->destroy = rhythm_template_destroy;
      self->name    = rhythm_template_name;
      self->onset   = rhythm_template_onset;
      self->beat    = rhythm_template_beat;
    
      /* initalize instance variables below here */
      /* return rhythm_template_destroy(self) on failure */
    }
  
  return (Rhythm*)self;
}

/*--------------------------------------------------------------------*/
void*      rhythm_template_destroy (void* SELF)
{
  Rhythm_Template* self = (Rhythm_Template*)SELF;
  if(self != NULL)
    {
      /* free any malloc-ed instance variables here */
    
      free(self);
    }
  return (Rhythm*) NULL;
}

/*--------------------------------------------------------------------*/
const char*  rhythm_template_name    (void* SELF)
{
  /* just return the name of the module for display */
  static const char* name = "Template";
  return name;
}

/*--------------------------------------------------------------------*/
void         rhythm_template_onset   (void* SELF, BTT* beat_tracker, unsigned long long sample_time)
{
  /* This will be called whenever an onset is detected
     sample_time tells you how many samples into the audio stream the
      onset occurred, and is probably a few milliseconds in the past
     you can do whatever you want with this information, or ignore it.
  */
  Rhythm_Template* self = (Rhythm_Template*)SELF;
  
}

/*--------------------------------------------------------------------*/
int          rhythm_template_beat    (void* SELF, BTT* beat_tracker, unsigned long long sample_time, rhythm_onset_t* returned_rhythm, int returned_rhythm_maxlen)
{
  /* This will be called whenever a beat is detected.
     You should generate one beat of rhythm and write it into returned_rhythm.
     returned_rhythm should contain values on [0~1) in ascending order
     that specify where within the beat the onset should occur. 0 indicates right now, 0.5
     is midway through the beat, etc. [0, 0.25, 0.5, 0.75] would be four 16th notes.
     Return the number of values written into returned_rhythm, not to exceed returned_rhythm_maxlen.
     sample_time tells you how many samples into the audio stream the beat occurrs,
     and might be in the past or future.
  */
  Rhythm_Template* self = (Rhythm_Template*)SELF;
  
  returned_rhythm[0].beat_time    = 0;
  returned_rhythm[0].strength     = 1;
  returned_rhythm[0].timbre_class = 0;

  returned_rhythm[1].beat_time    = 0.25;
  returned_rhythm[1].strength     = 0.1;
  returned_rhythm[1].timbre_class = 0;
  
  returned_rhythm[2].beat_time    = 0.5;
  returned_rhythm[2].strength     = 0.1;
  returned_rhythm[2].timbre_class = 0;
  
  returned_rhythm[3].beat_time    = 0.75;
  returned_rhythm[3].strength     = 1;
  returned_rhythm[3].timbre_class = 0;
  
  return 4;
  
  
}

