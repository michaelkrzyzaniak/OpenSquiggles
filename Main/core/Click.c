/*
 *  Click.h
 *  Make weird noises
 *
 *  Made by Michael Krzyzaniak at Arizona State University's
 *  School of Arts, Media + Engineering in Spring of 2013
 *  mkrzyzan@asu.edu
 */

#include "Click.h"

#define CLICK_KLOP_FREQ      (440 * 2 * M_PI / AU_SAMPLE_RATE)
#define CLICK_KLOP_DURATION  (AU_SAMPLE_RATE / 100)

/*--------------------------------------------------------------------*/
struct OpaqueClickStruct
{
  AUDIO_GUTS                  ;
  BOOL needs_click            ;
  int   klop_samples_remaining;
  float click_strength        ;
  float klop_strength         ;
  double klop_phase;
};

/*--------------------------------------------------------------------*/
int click_audio_callback         (void* SELF, auSample_t* buffer, int num_frames, int num_channels);
Click* click_destroy             (Click* self);


/*--------------------------------------------------------------------*/
Click* click_new()
{
  Click* self = (Click*) auAlloc(sizeof(*self), click_audio_callback, YES, 1);
  
  if(self != NULL)
    {
      self->destroy = (Audio* (*)(Audio*))click_destroy;
    }
  return self;
}


/*--------------------------------------------------------------------*/
Click* click_destroy(Click* self)
{
  if(self != NULL)
    {

    }
    
  return (Click*) NULL;
}

/*--------------------------------------------------------------------*/
void         click_click             (Click* self, float strength)
{
  if(strength > 1) strength = 1;
  if(strength < 0) strength = 0;
  
  self->click_strength = strength;
  self->needs_click = YES;
}

/*--------------------------------------------------------------------*/
void         click_klop             (Click* self, float strength)
{
  if(strength > 1) strength = 1;
  if(strength < 0) strength = 0;
  self->klop_strength = strength;
  self->klop_samples_remaining = CLICK_KLOP_DURATION;
  self->klop_phase = 0;
}

/*--------------------------------------------------------------------*/
int click_audio_callback(void* SELF, auSample_t* buffer, int num_frames, int num_channels)
{
  Click* self = (Click*)SELF;
  
  memset(buffer, 0, sizeof(*buffer) * num_frames * num_channels);

  int frame, chan;
  
  if(self->klop_samples_remaining>0)
    for(frame=0; frame<num_frames; frame+=num_channels)
      for(chan=0; chan<num_channels; chan++)
        {
          --self->klop_samples_remaining;
          buffer[frame + chan] = self->klop_strength * sin(self->klop_phase);
          self->klop_phase += CLICK_KLOP_FREQ;
        }
/*
  if(self->klop_samples_remaining>0)
    {
      buffer[0] = 1;
      self->needs_click = NO;
    }
*/
  if(self->needs_click)
    {
      buffer[0] = self->click_strength;
      self->needs_click = NO;
    }


  return  num_frames;
}

