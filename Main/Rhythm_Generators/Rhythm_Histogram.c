/*
 *  Rhythm_Histogram.c
 *  To use:
 *    if you want to make a module named "foo",
 *    search and replace "Histogram" with Foo (case sensitive)
 *    and "histogram" with "foo" (case sensitive).
 *    Then declare any variables you need in opaque_rhythm_foo_struct,
 *    initalize them in rhythm_foo_new,
 *    free them if necessary in rhythm_foo_destroy.
 *    Musical onsets will be reported in rhythm_histogram_onset
 *      (this is called automatically by the onset tracker)
 *    You should generate one beat of rhythm at a time, in rhythm_histogram_beat
 *      (this is called automatically by the beat tracker at the begining of each beat)
 *  mkrzyzan@asu.edu
 */

#include "Rhythm_Generators.h"

void  rhythm_inverse_histogram_set_is_inverse    (void* SELF, int is_inverse);
const char*  rhythm_histogram_name    (void* SELF);

/*--------------------------------------------------------------------*/
Rhythm* rhythm_histogram_new(BTT* beat_tracker)
{
  Rhythm* self = rhythm_inverse_histogram_new(beat_tracker);
  
  if(self != NULL)
    {
      rhythm_inverse_histogram_set_is_inverse(self, 0);
      self->name = rhythm_histogram_name;
    }
  
  return (Rhythm*)self;
}

/*--------------------------------------------------------------------*/
const char*  rhythm_histogram_name    (void* SELF)
{
  /* just return the name of the module for display */
  static const char* name = "Regular Histogram";
  return name;
}
