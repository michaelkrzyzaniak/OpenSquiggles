/*
 *  Rhythm_Silence.c
 *  To use:
 *    if you want to make a module named "foo",
 *    search and replace "Silence" with Foo (case sensitive)
 *    and "silence" with "foo" (case sensitive).
 *    Then declare any variables you need in opaque_rhythm_foo_struct,
 *    initalize them in rhythm_foo_new,
 *    free them if necessary in rhythm_foo_destroy.
 *    Musical onsets will be reported in rhythm_silence_onset
 *      (this is called automatically by the onset tracker)
 *    You should generate one beat of rhythm at a time, in rhythm_silence_beat
 *      (this is called automatically by the beat tracker at the begining of each beat)
 *  mkrzyzan@asu.edu
 */

#include "Rhythm_Generators.h"

/*--------------------------------------------------------------------*/
void*        rhythm_silence_destroy (void*);
const char*  rhythm_silence_name    (void*);
void         rhythm_silence_onset   (void*, BTT*, unsigned long long);
int          rhythm_silence_beat    (void*, BTT*, unsigned long long, rhythm_onset_t*, int);

/*--------------------------------------------------------------------*/
typedef struct opaque_rhythm_silence_struct
{
  RHYTHM_GENERATOR_SUBCLASS_GUTS ;
  
  /* add instance variables below here */
  
}Rhythm_Silence;

/*--------------------------------------------------------------------*/
Rhythm* rhythm_silence_new(BTT* beat_tracker)
{
  Rhythm_Silence* self = (Rhythm_Silence*) calloc(1, sizeof(*self));
  
  if(self != NULL)
    {
      self->destroy = rhythm_silence_destroy;
      self->name    = rhythm_silence_name;
      self->onset   = rhythm_silence_onset;
      self->beat    = rhythm_silence_beat;
    
      /* initalize instance variables below here */
      /* return rhythm_silence_destroy(self) on failure */
    
    }
  
  return (Rhythm*)self;
}

/*--------------------------------------------------------------------*/
void*      rhythm_silence_destroy (void* SELF)
{
  Rhythm_Silence* self = (Rhythm_Silence*)SELF;
  if(self != NULL)
    {
      /* free any malloc-ed instance variables here */
    
      free(self);
    }
  return (Rhythm*) NULL;
}

/*--------------------------------------------------------------------*/
const char*  rhythm_silence_name    (void* SELF)
{
  /* just return the name of the module for display */
  static const char* name = "Silence";
  return name;
}

/*--------------------------------------------------------------------*/
void         rhythm_silence_onset   (void* SELF, BTT* beat_tracker, unsigned long long sample_time)
{
  /* This will be called whenever an onset is detected
     sample_time tells you how many samples into the audio stream the
      onset occurred, and is probably a few milliseconds in the past
     you can do whatever you want with this information, or ignore it.
  */
  Rhythm_Silence* self = (Rhythm_Silence*)SELF;
  
}

/*--------------------------------------------------------------------*/
int          rhythm_silence_beat    (void* SELF, BTT* beat_tracker, unsigned long long sample_time, rhythm_onset_t* returned_rhythm, int returned_rhythm_maxlen)
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
  return 0;
  
}

