#include "Beep.h"

#include "../../Beat-and-Tempo-Tracking/src/fastsin.h"

#define MAX_NUM_NOTES 10

/*--------------------------------------------------------------------*/
struct OpaqueBeepStruct
{
  AUDIO_GUTS                     ;
  fastsin_t phase[MAX_NUM_NOTES] ;
  fastsin_t freq[MAX_NUM_NOTES]  ;
  int num_notes                  ;
};

/*--------------------------------------------------------------------*/
int   beep_audio_callback       (void* SELF, auSample_t* buffer, int num_frames, int num_channels);
Beep* beep_destroy              (Beep* self);


/*--------------------------------------------------------------------*/
Beep* beep_new()
{
  Beep* self = (Beep*) auAlloc(sizeof(*self), beep_audio_callback, YES, 1, 44100, 512, 4);
  
  if(self != NULL)
    {
      self->destroy = (Audio* (*)(Audio*))beep_destroy;
      beep_set_notes(self, NULL, 0);
    }
  return self;
}


/*--------------------------------------------------------------------*/
Beep* beep_destroy(Beep* self)
{
  if(self != NULL)
    {
      
    }
    
  return (Beep*) NULL;
}

/*--------------------------------------------------------------------*/
void beep_set_notes(Beep* self, int* midi_note_number, int num_notes)
{
  int i;
  for(i=0; i<num_notes; i++)
    self->freq[i] = AU_MIDI2FREQ(midi_note_number[i], self->sampleRate);

  self->num_notes = num_notes;
}

/*--------------------------------------------------------------------*/
int beep_audio_callback(void* SELF, auSample_t* buffer, int num_frames, int num_channels)
{
  Beep* self = SELF;
  
  //memset(buffer, 0, sizeof(*buffer) * num_frames * num_channels);

  int frame, chan, note;
  auSample_t sample;

  for(frame=0; frame<num_frames; frame++)
    {
      sample = 0;
      for(note=0; note<self->num_notes; note++)
        {
          sample += 0.5*fastsin(self->phase[note]);
          self->phase[note] += self->freq[note];
        }
      
      for(chan=0; chan<num_channels; chan++)
        *buffer++ = sample;
    }

  return  num_frames;
}

