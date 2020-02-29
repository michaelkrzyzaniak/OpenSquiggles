/*
 *  Rhythm_histogram.c
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
#include <math.h>

/*--------------------------------------------------------------------*/
void*        rhythm_histogram_destroy (void*);
const char*  rhythm_histogram_name    (void*);
void         rhythm_histogram_onset   (void*, BTT*, unsigned long long);
int          rhythm_histogram_beat    (void*, BTT*, unsigned long long, rhythm_onset_t*, int);

#define NUM_ONSET_TIMES           128
#define MAX_SUBDIVISIONS_PER_BEAT 8
#define MAX_NUM_BEATS             8
#define DEFAULT_SUBDIVISIONS_PER_BEAT 4
#define DEFAULT_NUM_BEATS             4
#define HISTOGRAM_MAX_LENGTH (MAX_SUBDIVISIONS_PER_BEAT * MAX_NUM_BEATS)
#define DEFAULT_DECAY_COEFFICIENT 0.75

/*--------------------------------------------------------------------*/
typedef struct opaque_rhythm_histogram_struct
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
  
  int                 num_beats;
  int                 subdivisions_per_beat;
  
  int                 is_inverse;
  double              nonlinear_exponent;
  
  
}Rhythm_Histogram;

/*--------------------------------------------------------------------*/
Rhythm* rhythm_histogram_new(BTT* beat_tracker)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*) calloc(1, sizeof(*self));
  
  if(self != NULL)
    {
      self->destroy = rhythm_histogram_destroy;
      self->name    = rhythm_histogram_name;
      self->onset   = rhythm_histogram_onset;
      self->beat    = rhythm_histogram_beat;
    
      /* initalize instance variables below here */
      self->onset_times = calloc(NUM_ONSET_TIMES, sizeof(*self->onset_times));
      if(self->onset_times == NULL) return rhythm_histogram_destroy(self);
    
      self->histogram = calloc(HISTOGRAM_MAX_LENGTH, sizeof(*self->histogram));
      if(self->histogram == NULL) return rhythm_histogram_destroy(self);
    
      int i;
      //for(i=0; i<HISTOGRAM_MAX_LENGTH; self->histogram[i++] = 0.5);      // all 0.5
      for(i=0; i<HISTOGRAM_MAX_LENGTH; self->histogram[i++] = random()%2); // 0 or 1 with 50% probability
    
      rhythm_histogram_set_is_inverse(self, 0);
      rhythm_histogram_set_num_beats  (self, DEFAULT_NUM_BEATS);
      rhythm_histogram_set_subdivisions_per_beat(self, DEFAULT_SUBDIVISIONS_PER_BEAT);
      rhythm_histogram_set_nonlinear_exponent(self, 1);
      rhythm_histogram_set_decay_coefficient(self, DEFAULT_DECAY_COEFFICIENT);
    }
  
  return (Rhythm*)self;
}

/*--------------------------------------------------------------------*/
void*      rhythm_histogram_destroy (void* SELF)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
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
void  rhythm_histogram_set_is_inverse    (void* SELF, int is_inverse)
{
  /* just return the name of the module for display */
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  self->is_inverse = (is_inverse) ? -1 : 1;
}

/*--------------------------------------------------------------------*/
int  rhythm_histogram_get_is_inverse    (void* SELF)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  return self->is_inverse != 1;
}

/*--------------------------------------------------------------------*/
void  rhythm_histogram_set_num_beats  (void* SELF, int num_beats)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  if(num_beats < 1) num_beats = 1;
  if(num_beats > MAX_NUM_BEATS) num_beats = MAX_NUM_BEATS;
  self->num_beats = num_beats;
}

/*--------------------------------------------------------------------*/
int   rhythm_histogram_get_num_beats  (void* SELF)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  return self->num_beats;
}

/*--------------------------------------------------------------------*/
void  rhythm_histogram_set_subdivisions_per_beat(void* SELF, int subdivisions)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  if(subdivisions < 1) subdivisions = 1;
  if(subdivisions > MAX_SUBDIVISIONS_PER_BEAT) subdivisions = MAX_SUBDIVISIONS_PER_BEAT;
  self->subdivisions_per_beat = subdivisions;
}

/*--------------------------------------------------------------------*/
int   rhythm_histogram_get_subdivisions_per_beat(void* SELF)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  return self->subdivisions_per_beat;
}

/*--------------------------------------------------------------------*/
void   rhythm_histogram_set_nonlinear_exponent(void* SELF, double k)
{
  //x >  0.5: (  1 + abs((2x-1))^k  ) / 2
  //x <= 0.5: (  1 - abs((2x-1))^k  ) / 2
  //switch plus and minus for inverse mode
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  if(k < 0) k = 0;
  self->nonlinear_exponent = k;
}

/*--------------------------------------------------------------------*/
double rhythm_histogram_get_nonlinear_exponent(void* SELF)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  return self->nonlinear_exponent;
}

/*--------------------------------------------------------------------*/
void   rhythm_histogram_set_decay_coefficient(void* SELF, double coeff)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  if(coeff < 0) coeff = 0;
  if(coeff > 1) coeff = 1;
  self->decay_coefficient = coeff;
}

/*--------------------------------------------------------------------*/
double rhythm_histogram_get_decay_coefficient(void* SELF)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  return self->decay_coefficient;
}

/*--------------------------------------------------------------------*/
const char*  rhythm_histogram_name    (void* SELF)
{
  /* just return the name of the module for display */
  static const char* name = "Histogram";
  return name;
}

/*--------------------------------------------------------------------*/
void         rhythm_histogram_onset   (void* SELF, BTT* beat_tracker, unsigned long long sample_time)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
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
float        rhythm_histogram_get_note_density   (Rhythm_Histogram* self)
{
  float density = 0;
  int i;
  int length = self->num_beats *  self->subdivisions_per_beat;
  for(i=0; i<length; density += self->histogram[i++]);
  
  density /= (double)length;
  
  return (self->is_inverse) ? 1.0-density : density;
}

/*--------------------------------------------------------------------*/
//try to maximize this value
float        rhythm_histogram_get_convergence_score   (Rhythm_Histogram* self)
{
  float result;
  
  float minimax = 2;
  float maximin = -1;
  int length = self->num_beats *  self->subdivisions_per_beat;
  int i;
  for(i=0; i<length; i++)
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
int          rhythm_histogram_beat    (void* SELF, BTT* beat_tracker, unsigned long long sample_time, rhythm_onset_t* returned_rhythm, int returned_rhythm_maxlen)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  
  int beat_duration = sample_time - self->prev_beat_time;
  if(beat_duration <= 0) return 0;
  
  unsigned long long prev_beat_time = self->prev_beat_time;
  self->prev_beat_time = sample_time;
  
  if(prev_beat_time == 0) return 0;
  
  int i, n=0;
  
  float onset_mask[MAX_SUBDIVISIONS_PER_BEAT] = {0};
  
  //get the onset mask
  while(self->num_onsets > 0)
    {
      //fprintf(stderr, "%llu, %llu, %i\t", self->onset_times[self->onsets_head_index], prev_beat_time, beat_duration);
    
      float beat_time = (int)(self->onset_times[self->onsets_head_index] - prev_beat_time) / (double)beat_duration;
      beat_time = round(beat_time * self->subdivisions_per_beat);
    
      if(beat_time >= self->subdivisions_per_beat)
        break;
      else if(beat_time >= 0)
        onset_mask[(int)beat_time] = 1;
      //else it is too old so let it get shifted out
  
      ++self->onsets_head_index; self->onsets_head_index %= NUM_ONSET_TIMES;
      --self->num_onsets;
    }

  //update and decay the histogram
  for(i=0; i<self->subdivisions_per_beat; i++)
    {
      self->histogram[self->histogram_index + i] *= self->decay_coefficient;
      self->histogram[self->histogram_index + i] += onset_mask[i] * (1.0-self->decay_coefficient);
      //fprintf(stderr, "%f\t", self->histogram[self->histogram_index + i]);
    }
  //fprintf(stderr, "\r\n");
  
  //fprintf(stderr, "Note Density: %f Convergence: %f\r\n", rhythm_histogram_get_note_density(self), rhythm_histogram_get_convergence_score(self));
  
  self->histogram_index += self->subdivisions_per_beat;
  self->histogram_index %= self->subdivisions_per_beat * self->num_beats;

  //generate a rhythm
  for(i=0; i<self->subdivisions_per_beat; i++)
    {
      float r = random() / (double)(RAND_MAX);
      float x = self->histogram[self->histogram_index + i];
      float nonliniarity = fabs(2*x-1);
      nonliniarity = pow(nonliniarity, self->nonlinear_exponent);
      nonliniarity += self->is_inverse;
      if(x < 0.5) nonliniarity *= -1;
      nonliniarity += 1;
      nonliniarity /= 2.0;

      if(nonliniarity > r)
        {
          returned_rhythm[n].beat_time    = i/(float)self->subdivisions_per_beat;
          returned_rhythm[n].strength     = -1;
          returned_rhythm[n].timbre_class = -1;
          ++n;
        }
    }
  return n;
}

