/*
 *  Rhythm_Random_Beat_From_List.c
 *  To use:
 *    if you want to make a module named "foo",
 *    search and replace "Random_Beat_From_List" with Foo (case sensitive)
 *    and "random_beat_from_list" with "foo" (case sensitive).
 *    Then declare any variables you need in opaque_rhythm_foo_struct,
 *    initalize them in rhythm_foo_new,
 *    free them if necessary in rhythm_foo_destroy.
 *    Musical onsets will be reported in rhythm_random_beat_from_list_onset
 *      (this is called automatically by the onset tracker)
 *    You should generate one beat of rhythm at a time, in rhythm_random_beat_from_list_beat
 *      (this is called automatically by the beat tracker at the begining of each beat)
 *  mkrzyzan@asu.edu
 */

#include "Rhythm_Generators.h"
#include <stdlib.h> //calloc

/*--------------------------------------------------------------------*/
void*        rhythm_random_beat_from_list_destroy (void*);
const char*  rhythm_random_beat_from_list_name    (void*);
void         rhythm_random_beat_from_list_onset   (void*, BTT*, unsigned long long);
int          rhythm_random_beat_from_list_beat    (void*, BTT*, unsigned long long, rhythm_onset_t*, int);

/*--------------------------------------------------------------------*/
const float rhythm_random_beat_from_list_list[][8] =
{
  { 0, },
  { 1, 0.5 },
  { 2, 0.5, 0.75 },
  { 2, 0.25, 0.5 },
  { 3, 0.25, 0.5, 0.75 },
  { 1, 0 },
  { 2, 0, 0.75 },
  { 2, 0, 0.5 },
  { 3, 0, 0.5, 0.75 },
  { 2, 0, 0.25 },
  { 3, 0, 0.25, 0.75 },
  { 3, 0, 0.25, 0.5 },
  { 4, 0, 0.25, 0.5, 0.75 },
  { 3, 0, 0.333, 0.666 },
  { 6, 0, 0.167, 0.333, 0.5, 0.667, 0.833 },
  { 4, 0, 0.167, 0.333, 0.5 },
  { 5, 0, 0.167, 0.333, 0.5, 0.75 },
  { 3, 0.5, 0.667, 0.833 },
  { 4, 0, 0.5, 0.667, 0.833 },
  { 5, 0, 0.25, 0.5, 0.667, 0.833 },
};

static const int rhythm_random_beat_from_list_list_length = sizeof(rhythm_random_beat_from_list_list) / sizeof(*rhythm_random_beat_from_list_list);

/*--------------------------------------------------------------------*/
typedef struct opaque_rhythm_random_beat_from_list_struct
{
  RHYTHM_GENERATOR_SUBCLASS_GUTS ;
  
  /* add instance variables below here */
  
}Rhythm_Random_Beat_From_List;

/*--------------------------------------------------------------------*/
Rhythm* rhythm_random_beat_from_list_new(BTT* beat_tracker)
{
  Rhythm_Random_Beat_From_List* self = (Rhythm_Random_Beat_From_List*) calloc(1, sizeof(*self));
  
  if(self != NULL)
    {
      self->destroy = rhythm_random_beat_from_list_destroy;
      self->name    = rhythm_random_beat_from_list_name;
      self->onset   = rhythm_random_beat_from_list_onset;
      self->beat    = rhythm_random_beat_from_list_beat;
    
      /* initalize instance variables below here */
      /* return rhythm_random_beat_from_list_destroy(self) on failure */
    }
  
  return (Rhythm*)self;
}

/*--------------------------------------------------------------------*/
void*      rhythm_random_beat_from_list_destroy (void* SELF)
{
  Rhythm_Random_Beat_From_List* self = (Rhythm_Random_Beat_From_List*)SELF;
  if(self != NULL)
    {
      /* free any malloc-ed instance variables here */
    
      free(self);
    }
  return (Rhythm*) NULL;
}

/*--------------------------------------------------------------------*/
const char*  rhythm_random_beat_from_list_name    (void* SELF)
{
  /* just return the name of the module for display */
  static const char* name = "Random_Beat_From_List";
  return name;
}

/*--------------------------------------------------------------------*/
void         rhythm_random_beat_from_list_onset   (void* SELF, BTT* beat_tracker, unsigned long long sample_time)
{
  Rhythm_Random_Beat_From_List* self = (Rhythm_Random_Beat_From_List*)SELF;
}

/*--------------------------------------------------------------------*/
//move this whole thing later
#include <math.h>
float       rhythm_default_strength_for_onset(float beat_time, int n)
{
  //returns the 1 over the denominator of the nearest rational
  //number to beat_time, considering only denominators not
  //greater than n. Uses some damn thing related to the Ford
  //circle packing. Good luck trying to understand how it works.
  
  int i;
  int num_a=0, denom_a=1, num_b=1, denom_b=1, num_c, denom_c;
  float a,c;
  
  for(i=0; i<n; i++)
    {
      a = num_a / (float)denom_a;
      if(a == beat_time)
        break;
      
      if((denom_a + denom_b) > n)
        break;

      num_c = num_a + num_b;
      denom_c = denom_a + denom_b;

      c = num_c / (float)denom_c;
  
      if(beat_time < c)
        {num_b=num_c; denom_b=denom_c;}
      else
        {num_a=num_c; denom_a=denom_c;}
    }

  if(fabs(c-beat_time) < fabs(a-beat_time))
     denom_a = denom_c;
  float b = num_b / (float)denom_b;
  if(fabs(b-beat_time) < fabs(a-beat_time))
     denom_a = denom_b;

  //return sqrt(1.0 / (float)denom_a);
  return 1.0 / (float)denom_a;
}

/*--------------------------------------------------------------------*/
int          rhythm_random_beat_from_list_beat    (void* SELF, BTT* beat_tracker, unsigned long long sample_time, rhythm_onset_t* returned_rhythm, int returned_rhythm_maxlen)
{
  Rhythm_Random_Beat_From_List* self = (Rhythm_Random_Beat_From_List*)SELF;
  
  int r = random() % rhythm_random_beat_from_list_list_length;
  int len = rhythm_random_beat_from_list_list[r][0];
  if(len > returned_rhythm_maxlen) len = returned_rhythm_maxlen;
  int i;

  
  for(i=0; i<len; i++)
    {
      returned_rhythm[i].beat_time = rhythm_random_beat_from_list_list[r][i+1];
      returned_rhythm[i].strength = rhythm_default_strength_for_onset(returned_rhythm[i].beat_time, 6);
      returned_rhythm[i].timbre_class = 1;
    }
  return len;
  
  
}

