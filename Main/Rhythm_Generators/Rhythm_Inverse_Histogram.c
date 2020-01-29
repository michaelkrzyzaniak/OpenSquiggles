/*
 *  Rhythm_inverse_histogram.c
 *  To use:
 *    if you want to make a module named "foo",
 *    search and replace "Inverse_Histogram" with Foo (case sensitive)
 *    and "inverse_histogram" with "foo" (case sensitive).
 *    Then declare any variables you need in opaque_rhythm_foo_struct,
 *    initalize them in rhythm_foo_new,
 *    free them if necessary in rhythm_foo_destroy.
 *    Musical onsets will be reported in rhythm_inverse_histogram_onset
 *      (this is called automatically by the onset tracker)
 *    You should generate one beat of rhythm at a time, in rhythm_inverse_histogram_beat
 *      (this is called automatically by the beat tracker at the begining of each beat)
 *  mkrzyzan@asu.edu
 */

#include "Rhythm_Generators.h"
#include <math.h>

/*--------------------------------------------------------------------*/
void*        rhythm_inverse_histogram_destroy (void*);
const char*  rhythm_inverse_histogram_name    (void*);
void         rhythm_inverse_histogram_onset   (void*, BTT*, unsigned long long);
int          rhythm_inverse_histogram_beat    (void*, BTT*, unsigned long long, rhythm_onset_t*, int);

#define NUM_ONSET_TIMES 128
#define SUBDIVIDIONS_PER_BEAT 4
#define NUM_BEATS 4
#define HISTOGRAM_LENGTH (SUBDIVIDIONS_PER_BEAT * NUM_BEATS)


/*--------------------------------------------------------------------*/
typedef struct opaque_rhythm_inverse_histogram_struct
{
  RHYTHM_GENERATOR_SUBCLASS_GUTS ;
  
  /* add instance variables below here */
  unsigned long long* onset_times;
  unsigned long long  prev_beat_time;
  int                 onsets_index;
  int                 onsets_head_index;
  int                 num_onsets;
  int                 histogram_index;
  
  float*              histogram;
  float               decay_coefficient;
  
  int                 is_inverse;
  
  
}Rhythm_Inverse_Histogram;

/*--------------------------------------------------------------------*/
Rhythm* rhythm_inverse_histogram_new(BTT* beat_tracker)
{
  Rhythm_Inverse_Histogram* self = (Rhythm_Inverse_Histogram*) calloc(1, sizeof(*self));
  
  if(self != NULL)
    {
      self->destroy = rhythm_inverse_histogram_destroy;
      self->name    = rhythm_inverse_histogram_name;
      self->onset   = rhythm_inverse_histogram_onset;
      self->beat    = rhythm_inverse_histogram_beat;
    
      /* initalize instance variables below here */
      self->onset_times = calloc(NUM_ONSET_TIMES, sizeof(*self->onset_times));
      if(self->onset_times == NULL) return rhythm_inverse_histogram_destroy(self);
    
      self->histogram = calloc(HISTOGRAM_LENGTH, sizeof(*self->histogram));
      if(self->histogram == NULL) return rhythm_inverse_histogram_destroy(self);
    
      int i;
      //for(i=0; i<HISTOGRAM_LENGTH; self->histogram[i++] = 0.5);      // all 0.5
      for(i=0; i<HISTOGRAM_LENGTH; self->histogram[i++] = random()%2); // 0 or 1 with 50% probability
    
      self->decay_coefficient = 0.75;
    
      self->is_inverse = 1;
      /* return rhythm_inverse_histogram_destroy(self) on failure */
    }
  
  return (Rhythm*)self;
}

/*--------------------------------------------------------------------*/
void*      rhythm_inverse_histogram_destroy (void* SELF)
{
  Rhythm_Inverse_Histogram* self = (Rhythm_Inverse_Histogram*)SELF;
  if(self != NULL)
    {
      /* free any malloc-ed instance variables here */
      if(self->onset_times != NULL)
        free(self->onset_times);
      if(self->histogram != NULL)
        free(self->histogram);
      free(self);
    }
  return (Rhythm*) NULL;
}

/*--------------------------------------------------------------------*/
void  rhythm_inverse_histogram_set_is_inverse    (void* SELF, int is_inverse)
{
  /* just return the name of the module for display */
  Rhythm_Inverse_Histogram* self = (Rhythm_Inverse_Histogram*)SELF;
  self->is_inverse = is_inverse;
}


/*--------------------------------------------------------------------*/
const char*  rhythm_inverse_histogram_name    (void* SELF)
{
  /* just return the name of the module for display */
  static const char* name = "Inverse Histogram";
  return name;
}

/*--------------------------------------------------------------------*/
void         rhythm_inverse_histogram_onset   (void* SELF, BTT* beat_tracker, unsigned long long sample_time)
{
  Rhythm_Inverse_Histogram* self = (Rhythm_Inverse_Histogram*)SELF;
  int adjustment = btt_get_beat_prediction_adjustment_audio_samples(beat_tracker);
  if(sample_time > adjustment) sample_time -= adjustment;// sample_time -= 64;
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
float        rhythm_inverse_histogram_get_note_density   (Rhythm_Inverse_Histogram* self)
{
  float density = 0;
  int i;
  for(i=0; i<HISTOGRAM_LENGTH; density += self->histogram[i++]);
  
  density /= (double)HISTOGRAM_LENGTH;
  
  return (self->is_inverse) ? 1.0-density : density;
}

/*--------------------------------------------------------------------*/
//try to maximize this value
float        rhythm_inverse_histogram_get_convergence_score   (Rhythm_Inverse_Histogram* self)
{
  float result;
  
  float minimax = 2;
  float maximin = -1;
  int i;
  for(i=0; i<HISTOGRAM_LENGTH; i++)
    {
      if(self->histogram[i] >= 0.5)
        {
          if(self->histogram[i] < minimax)
            minimax = self->histogram[i];
        }
      else
        {
          if(self->histogram[i] > maximin)
            maximin = self->histogram[i];
        }
    }
  
  if((minimax == 2)|| (maximin == -1))
    result = 0;
  else
    result = minimax - maximin;
  return result;
}

/*--------------------------------------------------------------------*/
int          rhythm_inverse_histogram_beat    (void* SELF, BTT* beat_tracker, unsigned long long sample_time, rhythm_onset_t* returned_rhythm, int returned_rhythm_maxlen)
{
  Rhythm_Inverse_Histogram* self = (Rhythm_Inverse_Histogram*)SELF;
  
  int beat_duration = sample_time - self->prev_beat_time;
  if(beat_duration <= 0) return 0;
  
  unsigned long long prev_beat_time = self->prev_beat_time;
  self->prev_beat_time = sample_time;
  
  if(prev_beat_time == 0) return 0;
  
  int i, n=0;
  
  float onset_mask[SUBDIVIDIONS_PER_BEAT] = {0};
  
  //get the onset mask
  while(self->num_onsets > 0)
    {
      //fprintf(stderr, "%llu, %llu, %i\t", self->onset_times[self->onsets_head_index], prev_beat_time, beat_duration);
    
      float beat_time = (int)(self->onset_times[self->onsets_head_index] - prev_beat_time) / (double)beat_duration;
      beat_time = round(beat_time * SUBDIVIDIONS_PER_BEAT);
    
      if(beat_time >= SUBDIVIDIONS_PER_BEAT)
        break;
      else if(beat_time >= 0)
        onset_mask[(int)beat_time] = 1;
      //else it is too old so let it get shifted out
  
      ++self->onsets_head_index; self->onsets_head_index %= NUM_ONSET_TIMES;
      --self->num_onsets;
    }

  //update and decay the histogram
  for(i=0; i<SUBDIVIDIONS_PER_BEAT; i++)
    {
      self->histogram[self->histogram_index + i] *= self->decay_coefficient;
      self->histogram[self->histogram_index + i] += onset_mask[i] * (1.0-self->decay_coefficient);
      //fprintf(stderr, "%f\t", self->histogram[self->histogram_index + i]);
    }
  //fprintf(stderr, "\r\n");
  
  fprintf(stderr, "Note Density: %f Convergence: %f\r\n", rhythm_inverse_histogram_get_note_density(self), rhythm_inverse_histogram_get_convergence_score(self));
  
  self->histogram_index += SUBDIVIDIONS_PER_BEAT;
  self->histogram_index %= HISTOGRAM_LENGTH;

  //generate a rhythm
  for(i=0; i<SUBDIVIDIONS_PER_BEAT; i++)
    {
      //float r = random() / (double)(RAND_MAX);
    
      float r = (self->histogram[self->histogram_index + i] >= 0.5) ? 0 : 1;
    
      int onset = (self->is_inverse) ? (r >= self->histogram[self->histogram_index + i]) : (r < self->histogram[self->histogram_index + i]);
      if(onset)
        {
          returned_rhythm[n].beat_time    = i/(float)SUBDIVIDIONS_PER_BEAT;
          returned_rhythm[n].strength     = -1;
          returned_rhythm[n].timbre_class = -1;
          ++n;
        }
    }

  return n;
}

