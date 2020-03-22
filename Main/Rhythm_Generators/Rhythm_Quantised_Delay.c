/*
 *  Rhythm_Quantized_Delay.c
 *  To use:
 *    if you want to make a module named "foo",
 *    search and replace "Quantized_Delay" with Foo (case sensitive)
 *    and "quantized_delay" with "foo" (case sensitive).
 *    Then declare any variables you need in opaque_rhythm_foo_struct,
 *    initalize them in rhythm_foo_new,
 *    free them if necessary in rhythm_foo_destroy.
 *    Musical onsets will be reported in rhythm_quantized_delay_onset
 *      (this is called automatically by the onset tracker)
 *    You should generate one beat of rhythm at a time, in rhythm_quantized_delay_beat
 *      (this is called automatically by the beat tracker at the begining of each beat)
 *  mkrzyzan@asu.edu
 */

#include "Rhythm_Generators.h"
#include "../extras/Quantizer.h"
#include <string.h> //memmove

/*--------------------------------------------------------------------*/
void*        rhythm_quantized_delay_destroy (void*);
const char*  rhythm_quantized_delay_name    (void*);
void         rhythm_quantized_delay_onset   (void*, BTT*, unsigned long long);
int          rhythm_quantized_delay_beat    (void*, BTT*, unsigned long long, rhythm_onset_t*, int);

#define NUM_IOIS               128
#define QUANTIZER_FIFO_LENGTH  8
#define NUM_ALREADY_QUANTIZED_IOIS ((NUM_IOIS) - (QUANTIZER_FIFO_LENGTH))
#define MAX_DELAY_BEATS        8
#define NUM_BEAT_TIMES        (MAX_DELAY_BEATS + 1)

/*--------------------------------------------------------------------*/
typedef struct opaque_rhythm_quantized_delay_struct
{
  RHYTHM_GENERATOR_SUBCLASS_GUTS ;
  
  /* add instance variables below here */
  Quantizer*          quantizer;
  unsigned long long  prev_onset_time;
  double*             quantized_iois;
  int*                onsets_per_beat;
  int                 beat_counter;
  float               beats_delay;
}Rhythm_Quantized_Delay;

/*--------------------------------------------------------------------*/
Rhythm* rhythm_quantized_delay_new(BTT* beat_tracker)
{
  Rhythm_Quantized_Delay* self = (Rhythm_Quantized_Delay*) calloc(1, sizeof(*self));
  
  if(self != NULL)
    {
      self->destroy = rhythm_quantized_delay_destroy;
      self->name    = rhythm_quantized_delay_name;
      self->onset   = rhythm_quantized_delay_onset;
      self->beat    = rhythm_quantized_delay_beat;
    
      /* initalize instance variables below here */
      
      self->quantizer = quantizer_new(QUANTIZER_FIFO_LENGTH);
      if(self->quantizer == NULL) return rhythm_quantized_delay_destroy(self);
      
      self->quantized_iois = calloc(NUM_IOIS, sizeof(*self->quantized_iois));
      if(self->quantized_iois == NULL) return rhythm_quantized_delay_destroy(self);

      self->onsets_per_beat = calloc(NUM_BEAT_TIMES, sizeof(*self->onsets_per_beat));
      if(self->onsets_per_beat == NULL) return rhythm_quantized_delay_destroy(self);
      
      self->beats_delay = 2;
      
      if(!quantizer_realtime_start(self->quantizer, 100000))
        return rhythm_quantized_delay_destroy(self);
      /* return rhythm_quantized_delay_destroy(self) on failure */
    }
  
  return (Rhythm*)self;
}

/*--------------------------------------------------------------------*/
void*      rhythm_quantized_delay_destroy (void* SELF)
{
  Rhythm_Quantized_Delay* self = (Rhythm_Quantized_Delay*)SELF;
  if(self != NULL)
    {
      /* free any malloc-ed instance variables here */
      quantizer_destroy(self->quantizer);
      if(self->quantized_iois != NULL)
        free(self->quantized_iois);
      if(self->onsets_per_beat != NULL)
        free(self->onsets_per_beat);
      free(self);
    }
  return (Rhythm*) NULL;
}

/*--------------------------------------------------------------------*/
const char*  rhythm_quantized_delay_name    (void* SELF)
{
  /* just return the name of the module for display */
  static const char* name = "Quantized_Delay";
  return name;
}

/*--------------------------------------------------------------------*/
void rhythm_quantized_delay_clear(void* SELF)
{
  Rhythm_Quantized_Delay* self = (Rhythm_Quantized_Delay*)SELF;
}

/*--------------------------------------------------------------------*/
void rhythm_quantized_delay_set_beats_delay(void* SELF, double beats_delay)
{
  Rhythm_Quantized_Delay* self = (Rhythm_Quantized_Delay*)SELF;
  
  if(beats_delay < 1)
    beats_delay = 1;
  if(beats_delay > MAX_DELAY_BEATS)
    beats_delay = MAX_DELAY_BEATS;
  self->beats_delay = beats_delay;
}

/*--------------------------------------------------------------------*/
double rhythm_quantized_delay_get_beats_delay(void* SELF)
{
  Rhythm_Quantized_Delay* self = (Rhythm_Quantized_Delay*)SELF;
  return self->beats_delay;
}

/*--------------------------------------------------------------------*/
void rhythm_quantized_delay_set_quantizer_update_interval(void* SELF, unsigned usecs)
{
  if(usecs < 1) usecs = 1;
  Rhythm_Quantized_Delay* self = (Rhythm_Quantized_Delay*)SELF;
  quantizer_realtime_set_update_interval(self->quantizer, usecs);
}

/*--------------------------------------------------------------------*/
unsigned rhythm_quantized_delay_get_quantizer_update_interval(void* SELF)
{
  Rhythm_Quantized_Delay* self = (Rhythm_Quantized_Delay*)SELF;
  return quantizer_realtime_get_update_interval(self->quantizer);
}


/*--------------------------------------------------------------------*/
void         rhythm_quantized_delay_onset   (void* SELF, BTT* beat_tracker, unsigned long long sample_time)
{
  Rhythm_Quantized_Delay* self = (Rhythm_Quantized_Delay*)SELF;

  if(self->prev_onset_time == 0)
    {
      self->prev_onset_time = sample_time;
      return;
    }

  double IOI = (signed int)(sample_time - self->prev_onset_time);
  self->prev_onset_time = sample_time;
  ++self->onsets_per_beat[self->beat_counter];
  
  unsigned len = NUM_ALREADY_QUANTIZED_IOIS * sizeof(*self->quantized_iois);
  memmove(self->quantized_iois, self->quantized_iois+1, len);
  
  IOI = quantizer_realtime_push(self->quantizer, IOI);
  self->quantized_iois[NUM_ALREADY_QUANTIZED_IOIS-1] = IOI;
}

/*--------------------------------------------------------------------*/
int          rhythm_quantized_delay_beat    (void* SELF, BTT* beat_tracker, unsigned long long sample_time, rhythm_onset_t* returned_rhythm, int returned_rhythm_maxlen)
{
  Rhythm_Quantized_Delay* self = (Rhythm_Quantized_Delay*)SELF;

  ++self->beat_counter; self->beat_counter %= NUM_BEAT_TIMES;
  rhythm_quantized_delay_onset(self, beat_tracker, sample_time);
  self->onsets_per_beat[self->beat_counter] = 0;

  int start_beat_ioi_index, end_beat_ioi_index=1;
  int n, i, p;
  for(i=1; i<self->beats_delay; i++)
    {
      p = self->beat_counter - i;
      if(p<0) p += NUM_BEAT_TIMES;
      end_beat_ioi_index += 1+self->onsets_per_beat[p];
    }
  
  start_beat_ioi_index = end_beat_ioi_index;
  p = self->beat_counter - self->beats_delay;
  if(p<0) p += NUM_BEAT_TIMES;
  start_beat_ioi_index += 1+self->onsets_per_beat[p];

  start_beat_ioi_index = NUM_IOIS - start_beat_ioi_index;
  end_beat_ioi_index   = NUM_IOIS - end_beat_ioi_index;
  if(start_beat_ioi_index < 0) return 0; //onsets too old, already shifted out
  n = end_beat_ioi_index - start_beat_ioi_index;
  if(n == 1) return 0; //no real onsets this beat, just one beat-onset
  
  if(end_beat_ioi_index >= NUM_ALREADY_QUANTIZED_IOIS)
    //the desired onsets are still in the quantizer fifo so lets pull them out
    quantizer_realtime_get_quantized_IOIs(self->quantizer, self->quantized_iois + NUM_ALREADY_QUANTIZED_IOIS);
  
  double onset_times[n];
  memcpy(&onset_times, self->quantized_iois+start_beat_ioi_index+1, sizeof(onset_times));
  quantizer_intervals_to_times(onset_times, n, 0, 1);
  
  double scale = onset_times[n-1];
  if(scale == 0) return 0;
  scale = 1.0 / scale;
  
  int num_onsets = n-1;
  if(num_onsets>returned_rhythm_maxlen)
    num_onsets = returned_rhythm_maxlen;
  
  for(i=0; i<num_onsets; i++)
    {
      returned_rhythm[i].beat_time     = onset_times[i] * scale;
      returned_rhythm[i].strength      = -1;
      returned_rhythm[i].timbre_class  = -1;
    }
  
  return num_onsets;
}

