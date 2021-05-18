/*------------------------------------------------------------------------
 _  _                         _
| || |__ _ _ _ _ __  ___ _ _ (_)______ _ _  __
| __ / _` | '_| '  \/ _ \ ' \| |_ / -_) '_|/ _|
|_||_\__,_|_| |_|_|_\___/_||_|_/__\___|_|(_)__|
------------------------------------------------------------------------
Written by Michael Krzyzaniak

----------------------------------------------------------------------*/
#include "Mono_Harmonizer.h"

#include <stdlib.h> //calloc
#include <math.h>  //log, exp, etc
//#include <pthread.h>  //thread, mutex

#include "../extras/dywapitchtrack.h"
#include "Mono_Harmonizer.h"

#define WINDOW_SIZE 2048

void mono_harmonizer_stft_process_callback(void* SELF, dft_sample_t* real, int N);

/*--------------------------------------------------------------------*/
struct opaque_mono_harmonizer_struct
{
  unsigned prev_note;

  void* notes_changed_callback_self;
  mono_harmonizer_notes_changed_callback_t notes_changed_callback;
  
  double*              audio_buffer;

  Organ_Pipe_Filter*   filter;
  dywapitchtracker     tracker;
  
  //pthread_mutex_t clear_mutex;
};


/*-----------------------------------------------------------------------*/
Mono_Harmonizer* mono_harmonizer_new()
{
  Mono_Harmonizer* self = calloc(1, sizeof(*self));

  if(self != NULL)
    {
      self->audio_buffer = malloc(WINDOW_SIZE * sizeof(*(self->audio_buffer)));
      if(self->audio_buffer == NULL)
        return mono_harmonizer_destroy(self);
      self->filter = organ_pipe_filter_new(WINDOW_SIZE, ORGAN_PIPE_FILTER_MODE_RESYNTHESIZED_AUDIO);
      if(self->filter == NULL)
        return mono_harmonizer_destroy(self);
        
      self->notes_changed_callback = NULL;
      dywapitch_inittracking(&self->tracker);
      //self->lowest_midi_note = 36;
    }
  return self;
}

/*-----------------------------------------------------------------------*/
Mono_Harmonizer*       mono_harmonizer_destroy               (Mono_Harmonizer* self)
{
  if(self != NULL)
    {
      self->filter = organ_pipe_filter_destroy(self->filter);
      if(self->audio_buffer == NULL)
        free(self->audio_buffer);
      free(self);
    }
  return (Mono_Harmonizer* ) NULL;
}

/*-----------------------------------------------------------------------*/
void mono_harmonizer_set_notes_changed_callback(Mono_Harmonizer* self, mono_harmonizer_notes_changed_callback_t callback,  void* callback_self)
{
  self->notes_changed_callback = callback;
  self->notes_changed_callback_self = callback_self;
}

/*-----------------------------------------------------------------------*/
void mono_harmonizer_init_state(Mono_Harmonizer* self)
{
  //pthread_mutex_lock(&self->clear_mutex);

  //pthread_mutex_unlock(&self->clear_mutex);
}

/*-----------------------------------------------------------------------*/
Organ_Pipe_Filter* mono_harmonizer_get_organ_pipe_filter(Mono_Harmonizer* self)
{
  return self->filter;
}

/*-----------------------------------------------------------------------*/
void mono_harmonizer_process_audio(Mono_Harmonizer* self, auSample_t* buffer, int num_frames)
{
  organ_pipe_filter_process(self->filter, buffer, num_frames, mono_harmonizer_stft_process_callback, self);
}

/*-----------------------------------------------------------------------*/
void __mono_harmonizer_stft_process_callback(void* SELF, dft_sample_t* real, int N)
{
  Mono_Harmonizer* self = SELF;
  
  if(N != WINDOW_SIZE) return;
  
  //because pitchtracker wants doubles and not floats
    
  int i;
  for(i=0; i<N; i++)
    {
      self->audio_buffer[i] = real[i];
      //fprintf(stderr, "%f\t%f\r\n", real[i], self->audio_buffer[i]);
    }
  //fprintf(stderr, "/r/n---------------------------\r\n");
 
  double current_freq = dywapitch_computepitch(&self->tracker, self->audio_buffer, 0, N);
  
  int current_note = (current_freq==0) ? 0 : round(AU_CPS2MIDI(current_freq));
  int min_note = 36;
  int max_note  = min_note + (12*5);
  if((current_note < min_note) || (current_note > max_note))
    current_note = 0;
  if(self->tracker._pitchConfidence <= 1)/*empirically ranges from 0 to 5*/
    current_note = 0;
    
  int num_curr_notes = (current_freq == 0) ? 0 : 1;
  if(current_note != self->prev_note)
    {
      fprintf(stderr, "MIDI: %i\tconfidence: %i\r\n", current_note, self->tracker._pitchConfidence);
      if(self->notes_changed_callback != NULL)
        self->notes_changed_callback(self->notes_changed_callback_self, &current_note, num_curr_notes);
    }
  self->prev_note = current_note;
}

Solenoid: 0	note: 70 (70.1 +- 0.1)	peak amplitude: 0.19	0 samples clipped
Solenoid: 1	note: 68 (68.0 +- 0.1)	peak amplitude: 0.17	0 samples clipped
Solenoid: 2	note: 60 (59.9 +- 0.1)	peak amplitude: 0.24	0 samples clipped
Solenoid: 3	note: 55 (55.1 +- 0.1)	peak amplitude: 0.11	0 samples clipped
Solenoid: 4	note: 54 (54.0 +- 0.1)	peak amplitude: 0.20	0 samples clipped
Solenoid: 5	note: 61 (61.0 +- 0.1)	peak amplitude: 0.24	0 samples clipped
Solenoid: 6	note: 65 (65.0 +- 0.1)	peak amplitude: 0.19	0 samples clipped
Solenoid: 7	note: 71 (71.0 +- 0.1)	peak amplitude: 0.21	0 samples clipped

/*-----------------------------------------------------------------------*/
void mono_harmonizer_stft_process_callback(void* SELF, dft_sample_t* real, int N)
{
  Mono_Harmonizer* self = SELF;
  static int i = 0;
  int notes[10];
  int num_notes = 0;
  //74, 68, 63, 57, 54, 61, 65, 70
  
  if(i==0)
    {
      notes[0] = 70;
      num_notes = 1;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }

  else if(i==44)
    {
      notes[0] = 68;
      num_notes = 1;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==88)
    {
      notes[0] = 60;
      num_notes = 1;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }

  if(i==132)
    {
      notes[0] = 55;
      num_notes = 1;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==176)
    {
      notes[0] = 54;
      num_notes = 1;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==220)
    {
      notes[0] = 61;
      num_notes = 1;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==264)
    {
      notes[0] = 65;
      num_notes = 1;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==308)
    {
      notes[0] = 71;
      num_notes = 1;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==352)
    {
      num_notes = 0;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  ++i;
}
