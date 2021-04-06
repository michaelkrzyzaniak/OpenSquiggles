/*----------------------------------------------------------------------
     ,'""`.      ,'""`.      ,'""`.      ,'""`.      ,'""`.
    / _  _ \    / _  _ \    / _  _ \    / _  _ \    / _  _ \
    |(@)(@)|    |(@)(@)|    |(@)(@)|    |(@)(@)|    |(@)(@)|
    )  __  (    )  __  (    )  __  (    )  __  (    )  __  (
   /,'))((`.\  /,'))((`.\  /,'))((`.\  /,'))((`.\  /,'))((`.\
  (( ((  )) ))(( ((  )) ))(( ((  )) ))(( ((  )) ))(( ((  )) ))
   `\ `)(' /'  `\ `)(' /'  `\ `)(' /'  `\ `)(' /'  `\ `)(' /'

----------------------------------------------------------------------*/

//OSX compile with:
//gcc opc.c core/*.c ../Robot_Communication_Framework/*.c ../Beat-and-Tempo-Tracking/src/*.c Rhythm_Generators/*.c extras/*.c -framework CoreMidi -framework Carbon -framework AudioToolbox -O2 -o opc

//Linux compile with:
//sudo apt-get install libasound2-dev
//gcc opc.c core/*.c ../Robot_Communication_Framework/*.c ../Beat-and-Tempo-Tracking/src/*.c Rhythm_Generators/*.c extras/*.c -lasound -lm -lpthread -lrt -O2 -o opc

#define __OPC_VERSION__ "0.1"

#define clip(in, val, min, max) in = (val < min) ? min : (val > max) ? max : val;

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h> //mkdir

#include "core/AudioSuperclass.h"
#include "extras/Params.h"
#include "extras/MKAiff.h"
#include "../Robot_Communication_Framework/Robot_Communication.h"
#include "../Beat-and-Tempo-Tracking/src/DFT.h"

#define PARAMS_DIR ".squiggles_notes"
#define PARAMS_FILENAME "squiggles_notes.xml" //it will go in home directory
#define NUM_SOLENOIDS 8
#define SAMPLE_LENGTH (1<<13)


/*--------------------------------------------------------------------*/
typedef void (*notify_when_done_recording_t)(void* SELF, MKAiff* aiff, unsigned samples_clipped);

/*--------------------------------------------------------------------*/
typedef struct opaque_sampler_calibrator_struct
{
  AUDIO_GUTS;
  MKAiff* aiff;
  
  unsigned samples_recorded;
  unsigned samples_clipped;
  
  notify_when_done_recording_t notify_when_done_callback;
  void* notify_when_done_self;
  
  
}Sampler_Calibrator;


/*--------------------------------------------------------------------*/
Sampler_Calibrator* sampler_calibrator_destroy (Sampler_Calibrator* self);
void sampler_calibrator_start_recording(Sampler_Calibrator* self, notify_when_done_recording_t notify_when_done_callback, void* notify_when_done_self);
int sampler_calibrator_audio_callback(void* SELF, auSample_t* buffer, int num_frames, int num_channels);
Sampler_Calibrator* sampler_calibrator_destroy(Sampler_Calibrator* self);

/*--------------------------------------------------------------------*/
Sampler_Calibrator* sampler_calibrator_new()
{
  Sampler_Calibrator* self = (Sampler_Calibrator*) auAlloc(sizeof(*self), sampler_calibrator_audio_callback, NO, 2);
  self->destroy = (Audio* (*)(Audio*))sampler_calibrator_destroy;
  
  if(self != NULL)
    {
      self->aiff = aiffWithDurationInSamples(1, AU_SAMPLE_RATE, 16, SAMPLE_LENGTH);
      if(self->aiff == NULL)
        return (Sampler_Calibrator*)auDestroy((Audio*)self);
      
      self->samples_recorded = SAMPLE_LENGTH;
      self->samples_clipped = 0;
    }
  
  return (Sampler_Calibrator*)self;
}

/*--------------------------------------------------------------------*/
Sampler_Calibrator* sampler_calibrator_destroy(Sampler_Calibrator* self)
{
  if(self != NULL)
    {
      aiffDestroy(self->aiff);
      free(self);
    }
  return self;
}

/*--------------------------------------------------------------------*/
void sampler_calibrator_start_recording(Sampler_Calibrator* self, notify_when_done_recording_t notify_when_done_callback, void* notify_when_done_self)
{
  //mutex lock
  self->notify_when_done_callback = notify_when_done_callback;
  self->notify_when_done_self = notify_when_done_self;
  aiffRewindPlayheadToBeginning(self->aiff);
  aiffRemoveSamplesAtPlayhead(self->aiff, aiffDurationInSamples(self->aiff));
  
  self->samples_recorded = 0;
  self->samples_clipped = 0;
  //mutex unlock
}

/*--------------------------------------------------------------------*/
int sampler_calibrator_audio_callback(void* SELF, auSample_t* buffer, int num_frames, int num_channels)
{
  Sampler_Calibrator* self = (Sampler_Calibrator*)SELF;
  int frame, channel;
  auSample_t samp = 0;
  
  
  //mix to mono without correcting amplitude
  for(frame=0; frame<num_frames; frame++)
    {
      samp = 0;
      for(channel=0; channel<num_channels; channel++)
        samp += buffer[frame * num_channels + channel];
      buffer[frame] = samp;
  }

  if(self->samples_recorded < SAMPLE_LENGTH)
    {
      int samples_needed = SAMPLE_LENGTH - self->samples_recorded;
      int samples_to_add = (samples_needed > num_frames) ? num_frames : samples_needed;
      
      self->samples_clipped += aiffAddFloatingPointSamplesAtPlayhead(self->aiff, buffer, samples_to_add, aiffFloatSampleType, aiffYes);
      
      self->samples_recorded += samples_to_add;
      
      if(self->samples_recorded >= SAMPLE_LENGTH)
        self->notify_when_done_callback(self->notify_when_done_self, self->aiff, self->samples_clipped);
    }

  return  num_frames;
}

/*--------------------------------------------------------------------*/
typedef struct main_struct
{
  float* real;
  float* imag;
  float* window;
  Params* params;
  const char* home;
  int i;
  int is_done_recording;
}globals_t;

/*--------------------------------------------------------------------*/
Params* main_init_params(const char* home)
{
  char *dir_string;
  char *filename_string;
  asprintf(&dir_string, "%s/%s", home, PARAMS_DIR);
  asprintf(&filename_string, "%s/%s/%s", home, PARAMS_DIR, PARAMS_FILENAME);
  mkdir(dir_string, 0777);
  Params* p = params_new(filename_string);
  free(dir_string);
  free(filename_string);
  return p;
}

/*--------------------------------------------------------------------*/
void main_notify_when_done_recording (void* SELF, MKAiff* aiff, unsigned samples_clipped)
{
  globals_t* globals = (globals_t*)SELF;
  
  int i, argmax;
  char *filename_string, *param_string;
  
  aiffRewindPlayheadToBeginning(aiff);
  aiffReadFloatingPointSamplesAtPlayhead(aiff, globals->real, SAMPLE_LENGTH, aiffYes);
  
  float max_sample = 0;
  for(i=0; i<SAMPLE_LENGTH; i++)
    {
      float test = fabs(globals->real[i]);
      if(test > max_sample)
        max_sample = test;
    }
  
  dft_apply_window(globals->real, globals->window, SAMPLE_LENGTH);
  dft_real_forward_dft(globals->real, globals->imag, SAMPLE_LENGTH);
  dft_rect_to_polar(globals->real, globals->imag, SAMPLE_LENGTH/2);
  
  //actual pipes range from 52 to 75
  int start_midi = 36;
  int end_midi = 96;
  
  int start_bin = dft_bin_of_frequency(AU_MIDI2CPS(start_midi), AU_SAMPLE_RATE, SAMPLE_LENGTH);
  int end_bin   = dft_bin_of_frequency(AU_MIDI2CPS(end_midi), AU_SAMPLE_RATE, SAMPLE_LENGTH);
  
  argmax = start_bin;
  //todo:check end_bin < sample_length / 2
  
  for(i=start_bin+1; i<end_bin; i++)
    {
      if(globals->real[i] > globals->real[argmax])
        argmax = i;
    }
  
  double freq = dft_frequency_of_bin(argmax, AU_SAMPLE_RATE, SAMPLE_LENGTH);
  int midi = round(AU_CPS2MIDI(freq));
  
  double freq_range_min = dft_frequency_of_bin(argmax-0.5, AU_SAMPLE_RATE, SAMPLE_LENGTH);
  double freq_range_max = dft_frequency_of_bin(argmax+0.5, AU_SAMPLE_RATE, SAMPLE_LENGTH);
  
  
  asprintf(&filename_string, "%s/%s/solenoid_%i_sample.aiff", globals->home, PARAMS_DIR, globals->i);
  asprintf(&param_string,     "solenoid_%i_note", globals->i);
  
  aiffSaveWithFilename(aiff, filename_string);
  //initalize only if it dosen't already exist
  params_init_int(globals->params, param_string, -1);
  params_set_int(globals->params, param_string, midi);
  
  fprintf(stderr, "solenoid: %i\tnote: %i (%.2f - %.2f)\tpeak amplitude: %.2f\t%u samples clipped\r\n", globals->i, midi, AU_CPS2MIDI(freq_range_min), AU_CPS2MIDI(freq_range_max), max_sample, samples_clipped);
  
  free(filename_string);
  free(param_string);
  
  globals->is_done_recording = 1;
}

/*--------------------------------------------------------------------*/
void  main_robot_message_received_callback(void* SELF, char* message, robot_arg_t args[], int num_args)
{

}

/*--------------------------------------------------------------------*/
int main(void)
{

  //defined globally because reasons
  Sampler_Calibrator* sampler = sampler_calibrator_new();
  if(sampler == NULL)
    {fprintf(stderr, "unable to make sampler object\r\n"); return -1;}
  
  Robot* robot = robot_new(main_robot_message_received_callback, NULL);
  if(robot == NULL)
    {fprintf(stderr, "unable to make robot object\r\n"); return -1;}
    
  globals_t globals;
  
  globals.home = getenv("HOME");
  
  globals.params = main_init_params(globals.home);
  if(globals.params == NULL)
    {fprintf(stderr, "unable to make params object\r\n"); return -1;}
      
  globals.real = calloc(SAMPLE_LENGTH, sizeof(*globals.real));
  if(globals.real == NULL)
    {fprintf(stderr, "unable to make real buffer\r\n"); return -1;}

  globals.imag = calloc(SAMPLE_LENGTH, sizeof(*globals.imag));
  if(globals.imag == NULL)
    {fprintf(stderr, "unable to make imag buffer\r\n"); return -1;}
    
  globals.window = calloc(SAMPLE_LENGTH, sizeof(*globals.window));
  if(globals.window == NULL)
    {fprintf(stderr, "unable to make window buffer\r\n"); return -1;}

  dft_init_blackman_window(globals.window, SAMPLE_LENGTH);
  
  robot_send_message(robot, robot_cmd_set_sustain_mode, 1);
  
  auPlay((Audio*)sampler);
  
  int i;
  for(i=0; i<NUM_SOLENOIDS; i++)
    {
      globals.i = i;
      globals.is_done_recording = 0;
      robot_send_message(robot, robot_cmd_note_on, i);
      usleep(500000);
      
      sampler_calibrator_start_recording(sampler, main_notify_when_done_recording, &globals);
      
      while(!globals.is_done_recording)
        usleep(10000);
      
      robot_send_message(robot, robot_cmd_note_off, i);
    
    }

  robot_send_message(robot, robot_cmd_all_notes_off);
  
  auPause((Audio*)sampler);
  
  free(globals.window);
  free(globals.imag);
  free(globals.real);
  //robot_destroy(robot);
  sampler_calibrator_destroy(sampler);
  
  fprintf(stderr, "Done.\r\n");
  
  return 0;
}

