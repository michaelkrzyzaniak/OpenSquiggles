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
int          rhythm_two_beat_delay_beat    (void*, BTT*, unsigned long long, rhythm_onset_t*, int);

#define NUM_ONSET_TIMES 128
#define NUM_BEAT_TIMES  8

/*--------------------------------------------------------------------*/
typedef struct opaque_rhythm_two_beat_delay_struct
{
  RHYTHM_GENERATOR_SUBCLASS_GUTS ;
  
  /* add instance variables below here */
  unsigned long long* onset_times;
  unsigned long long* beat_times;
  int                 onsets_index;
  int                 beats_index;
  int                 onsets_head_index;
  int                 num_onsets;
  float               beats_delay;
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
      self->onset_times = calloc(NUM_ONSET_TIMES, sizeof(*self->onset_times));
      if(self->onset_times == NULL) return rhythm_two_beat_delay_destroy(self);

      self->beat_times = calloc(NUM_BEAT_TIMES, sizeof(*self->beat_times));
      if(self->beat_times == NULL) return rhythm_two_beat_delay_destroy(self);
      
      self->beats_delay = 2;
      
      
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
      if(self->onset_times != NULL)
        free(self->onset_times);
      if(self->beat_times != NULL)
        free(self->beat_times);
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
//TESTING ONLY!!!!!!!!!!!!!
unsigned long long prev_beat_test = 0;

/*--------------------------------------------------------------------*/
void         rhythm_two_beat_delay_onset   (void* SELF, BTT* beat_tracker, unsigned long long sample_time)
{
  Rhythm_Two_Beat_Delay* self = (Rhythm_Two_Beat_Delay*)SELF;
  int adjustment = btt_get_beat_prediction_adjustment_audio_samples(beat_tracker);
  if(sample_time > adjustment) sample_time -= adjustment;// sample_time -= 64;
  fprintf(stderr, "discrepancy %lli\r\n", (signed long long)sample_time - (signed long long)prev_beat_test);
  self->onset_times[self->onsets_index] = sample_time;
  ++self->onsets_index; self->onsets_index %= NUM_ONSET_TIMES;
  ++self->num_onsets;
  
  
  /* if buffer is full, shift out the oldest samples */
  if(self->num_onsets > NUM_ONSET_TIMES)
    {
      self->onsets_head_index += (self->num_onsets - NUM_ONSET_TIMES);
      self->onsets_head_index %= NUM_ONSET_TIMES;
      self->num_onsets = NUM_ONSET_TIMES;
    }
}

/*--------------------------------------------------------------------*/
void rhythm_two_beat_delay_clear()
{

}

/*--------------------------------------------------------------------*/
int          rhythm_two_beat_delay_beat    (void* SELF, BTT* beat_tracker, unsigned long long sample_time, rhythm_onset_t* returned_rhythm, int returned_rhythm_maxlen)
{
  Rhythm_Two_Beat_Delay* self = (Rhythm_Two_Beat_Delay*)SELF;
  
  //fprintf(stderr, "beat_time %llu\r\n", sample_time);
  
  prev_beat_test = sample_time;
  
  self->beat_times[self->beats_index] = sample_time;
  int start_beats_index = (self->beats_index + NUM_BEAT_TIMES - (int)self->beats_delay    ) % NUM_BEAT_TIMES;
  int end_beats_index   = (self->beats_index + NUM_BEAT_TIMES - (int)self->beats_delay + 1) % NUM_BEAT_TIMES;
  ++self->beats_index; self->beats_index %= NUM_BEAT_TIMES;
  
  int beat_duration = self->beat_times[end_beats_index] - self->beat_times[start_beats_index];
  
  if(beat_duration < 0) return 0;
  
  /*-----------------------------------*/
  //testing only!!!!!!!!!!!!!!!!!!!!!!!!!!
  /*
  returned_rhythm[0].beat_time = 0;
  returned_rhythm[0].strength  = -1;
  returned_rhythm[0].timbre_class = 0;
  return 1;
  */
  /*-----------------------------------*/

  int n=0;
  while(self->num_onsets > 0)
    {
      if(self->onset_times[self->onsets_head_index] >= self->beat_times[end_beats_index])
        break;
      else if(self->onset_times[self->onsets_head_index] >= self->beat_times[start_beats_index])
        {
          returned_rhythm[n].beat_time    = (self->onset_times[self->onsets_head_index] - self->beat_times[start_beats_index]) / (float)beat_duration;
          returned_rhythm[n].strength     = 1;
          returned_rhythm[n].timbre_class = 0;
          ++n;
        }
      else //the onset is older than the start beat, so just ignore it and let it get shifted out
        {
          //returned_rhythm[n].beat_time    = (self->onset_times[self->onsets_head_index] - self->beat_times[start_beats_index]) / (float)beat_duration;
          //returned_rhythm[n].strength     = -1;
          //returned_rhythm[n].timbre_class = 0;
          //++n;
        }
      ++self->onsets_head_index; self->onsets_head_index %= NUM_ONSET_TIMES;
      --self->num_onsets;
    }
  
  return n;
}

