/*
 *  Synth.h
 *  Make weird noises
 *
 *  Made by Michael Krzyzaniak at Arizona State University's
 *  School of Arts, Media + Engineering in Spring of 2013
 *  mkrzyzan@asu.edu
 */

#include "HarmonizerController.h"
#include "Beep.h"
#include "constants.h"
#include "../extras/Params.h"


HarmonizerController* harmonizer_controller_destroy (HarmonizerController* self);
void harmonizer_controller_notes_changed_callback(void* SELF, int* midi_note, int num_notes);
int  harmonizer_controller_init_midi_note_params(HarmonizerController* self);
void harmonizer_controller_message_recd_from_robot(void* self, char* message, robot_arg_t args[], int num_args);
int  harmonizer_controller_audio_callback(void* SELF, auSample_t* buffer, int num_frames, int num_channels);

/*--------------------------------------------------------------------*/
struct OpaqueHarmonizerControllerStruct
{

  AUDIO_GUTS;
  int              harmonizer;
  Poly_Harmonizer* harmonizer_1;
  LSTM_Harmonizer* harmonizer_2;
  Beep*            beep;
  int              sounding_notes[OP_NUM_SOLENOIDS];
  int              midi_notes[OP_NUM_SOLENOIDS];
  Params*          params;
  Robot*           robot;
};

/*--------------------------------------------------------------------*/
HarmonizerController* harmonizer_controller_new ()
{
  HarmonizerController* self = (HarmonizerController*) auAlloc(sizeof(*self), harmonizer_controller_audio_callback, NO, 1, 44100, 512, 6);
  if(self != NULL)
    {
      self->destroy = (Audio* (*)(Audio*))harmonizer_controller_destroy;

      //self->harmonizer = poly_harmonizer_new("matrices/Multiple_F0");
      self->harmonizer_1 = poly_harmonizer_new(44100);
      if(self->harmonizer_1 == NULL)
        return (HarmonizerController*)auDestroy((Audio*)self);
        
      self->harmonizer_2 = lstm_harmonizer_new("LSTM_Matrices");
      if(self->harmonizer_2 == NULL)
        return (HarmonizerController*)auDestroy((Audio*)self);
        
      self->harmonizer = 2;
      self->beep = beep_new();
      //if(self->beep == NULL)
        //return (HarmonizerController*)auDestroy((Audio*)self);
        
      poly_harmonizer_set_notes_changed_callback(self->harmonizer_1, harmonizer_controller_notes_changed_callback, self);
      lstm_harmonizer_set_notes_changed_callback(self->harmonizer_2, harmonizer_controller_notes_changed_callback, self);

      
      char *filename_string;
      asprintf(&filename_string, "%s/%s/%s", getenv("HOME"), OP_PARAMS_DIR, OP_PARAMS_FILENAME);
      self->params = params_new(filename_string);
      free(filename_string);
      if(self->params == NULL)
        return (HarmonizerController*)auDestroy((Audio*)self);
  
       if(!harmonizer_controller_init_midi_note_params(self))
         {fprintf(stderr, "Robot does not know what notes it has. Run opc first.\r\n"); return (HarmonizerController*)auDestroy((Audio*)self);}
      
      self->robot = robot_new(harmonizer_controller_message_recd_from_robot, self);
      if(self->robot == NULL)
        return (HarmonizerController*)auDestroy((Audio*)self);
      
      //sleep(1);
      robot_send_message(self->robot, robot_cmd_get_firmware_version);
      robot_send_message(self->robot, robot_cmd_set_sustain_mode, 1);
      
      //there should be a play callback that I can intercept and do this there.
      if(self->beep)
        auPlay((Audio*)self->beep);
    }
  return self;
}

/*--------------------------------------------------------------------*/
HarmonizerController* harmonizer_controller_destroy (HarmonizerController* self)
{
  auDestroy((Audio*)self);
  if(self != NULL)
    {
      if(self->robot != NULL)
        robot_send_message(self->robot, robot_cmd_all_notes_off);
      
      self->harmonizer_1 = poly_harmonizer_destroy(self->harmonizer_1);
      self->harmonizer_2 = lstm_harmonizer_destroy(self->harmonizer_2);
      //self->robot = robot_destroy(self->robot);
      auDestroy((Audio*)self->beep);
    }
  
  return (HarmonizerController*) NULL;
}

/*--------------------------------------------------------------------*/
void harmonizer_controller_clear  (HarmonizerController*  self)
{
  robot_send_message(self->robot, robot_cmd_all_notes_off);
  if(self->beep)
    beep_set_notes(self->beep, NULL, 0);
  poly_harmonizer_init_state(self->harmonizer_1);
  lstm_harmonizer_init_state(self->harmonizer_2);
}

/*--------------------------------------------------------------------*/
/*
Robot*            harmonizer_controller_get_robot (HarmonizerController* self)
{
  return self->robot;
}
*/

/*--------------------------------------------------------------------*/
void harmonizer_controller_message_recd_from_robot(void* self, char* message, robot_arg_t args[], int num_args)
{
  //robot_debug_print_message(message, args, num_args);
  switch(robot_hash_message(message))
    {
      case robot_hash_reply_firmware_version:
        if(num_args == 2)
          fprintf(stderr, "Teensy is running firmware version %i.%i\r\n", robot_arg_to_int(&args[0]), robot_arg_to_int(&args[1]));
        break;
      
      default: break;
    }
}

/*--------------------------------------------------------------------*/
int harmonizer_controller_init_midi_note_params(HarmonizerController* self)
{
  self->midi_notes[0] = params_get_int(self->params, "solenoid_0_note", -1);
  self->midi_notes[1] = params_get_int(self->params, "solenoid_1_note", -1);
  self->midi_notes[2] = params_get_int(self->params, "solenoid_2_note", -1);
  self->midi_notes[3] = params_get_int(self->params, "solenoid_3_note", -1);
  self->midi_notes[4] = params_get_int(self->params, "solenoid_4_note", -1);
  self->midi_notes[5] = params_get_int(self->params, "solenoid_5_note", -1);
  self->midi_notes[6] = params_get_int(self->params, "solenoid_6_note", -1);
  self->midi_notes[7] = params_get_int(self->params, "solenoid_7_note", -1);
  
  return (self->midi_notes[7] == -1) ? 0 : 1;
}

/*--------------------------------------------------------------------*/
int harmonizer_controller_midi_to_solenoid_number(HarmonizerController* self, int midi_note)
{
  //todo: put these in a hash table
  //make array of length 127 with most erntries -1?
  int result  = -1;
  int i;
  for(i=0; i<OP_NUM_SOLENOIDS; i++)
    if(self->midi_notes[i] == midi_note)
      {
        result = i;
        break;
      }
  return result;
}

/*--------------------------------------------------------------------*/
void harmonizer_controller_notes_changed_callback(void* SELF, int* midi_notes, int num_notes)
{
  HarmonizerController* self = SELF;
  
  int i;
  for(i=0; i<OP_NUM_SOLENOIDS; i++)
    self->sounding_notes[i] = 0;

  int harmony_notes[8];
  int num_harmony_notes = 0;
  
  if(num_notes == 0)
    {
      num_harmony_notes = 0;
    }
  /*
  else if(midi_notes[num_notes-1] == 76)
    {
      harmony_notes[0] = 52;
      harmony_notes[1] = 59;
      harmony_notes[2] = 64;
      harmony_notes[3] = 68;
      
      harmony_notes[0] = 52;
      num_harmony_notes = 1;
    }
  else if(midi_notes[num_notes-1] == 83)
    {
      harmony_notes[0] = 53;
      harmony_notes[1] = 57;
      harmony_notes[2] = 62;
      harmony_notes[3] = 71;
      
      harmony_notes[0] = 59;
      num_harmony_notes = 1;
    }
  else if(midi_notes[num_notes-1] == 91)
    {
      harmony_notes[0] = 55;
      harmony_notes[1] = 59;
      harmony_notes[2] = 64;
      harmony_notes[3] = 71;
      
      harmony_notes[0] = 67;
      num_harmony_notes = 1;
    }
  else if(midi_notes[num_notes-1] == 96)
    {
      harmony_notes[0] = 57;
      harmony_notes[1] = 60;
      harmony_notes[2] = 64;
      harmony_notes[3] = 69;
      
      harmony_notes[0] = 72;
      num_harmony_notes = 1;
    }
  else if(midi_notes[num_notes-1] == 92)
    {
      harmony_notes[0] = 59;
      harmony_notes[1] = 62;
      harmony_notes[2] = 65;
      harmony_notes[3] = 68;
      
      harmony_notes[0] = 68;
      num_harmony_notes = 1;
    }
  else if(midi_notes[num_notes-1] == 88)
    {
      harmony_notes[0] = 58;
      harmony_notes[1] = 61;
      harmony_notes[2] = 64;
      harmony_notes[3] = 66;
      
      harmony_notes[0] = 64;
      num_harmony_notes = 1;
    }
  else if(midi_notes[num_notes-1] == 79)
    {
      harmony_notes[0] = 58;
      harmony_notes[1] = 61;
      harmony_notes[2] = 63;
      harmony_notes[3] = 67;
      
      harmony_notes[0] = 55;
      num_harmony_notes = 1;
    }
  else if(midi_notes[num_notes-1] == 84)
    {
      harmony_notes[0] = 56;
      harmony_notes[1] = 60;
      harmony_notes[2] = 63;
      harmony_notes[3] = 68;
      
      harmony_notes[0] = 60;
      num_harmony_notes = 1;
    }
  */
  else
    {
      harmony_notes[0] = midi_notes[0];
      num_harmony_notes = 1;
    }
  //for(i=0; i<num_notes; i++)
  for(i=0; i<num_harmony_notes; i++)
    {
      while(midi_notes[i] > 75)
        midi_notes[i] -= 12;
      while (midi_notes[i] < 52)
        midi_notes[i] += 12;
        
      //int solenoid = harmonizer_controller_midi_to_solenoid_number(self, midi_notes[i]);
      int solenoid = harmonizer_controller_midi_to_solenoid_number(self, harmony_notes[i]);
      
      if(solenoid >= 0)
        self->sounding_notes[solenoid] = 1;
    }
    
  //todo: only set the notes that changed;
  for(i=0; i<OP_NUM_SOLENOIDS; i++)
    if(self->sounding_notes[i] == 1)
      robot_send_message(self->robot, robot_cmd_note_on, i);
    else
      robot_send_message(self->robot, robot_cmd_note_off, i);

  Organ_Pipe_Filter* filter;
  if(self->harmonizer == 1)
    filter = poly_harmonizer_get_organ_pipe_filter(self->harmonizer_1);
  else
    filter = lstm_harmonizer_get_organ_pipe_filter(self->harmonizer_2);
    
  organ_pipe_filter_notify_sounding_notes(filter, self->sounding_notes);
  
  if(self->beep)
    //beep_set_notes(self->beep, harmony_notes, num_harmony_notes);
    beep_set_notes(self->beep, midi_notes, num_notes);
}

/*--------------------------------------------------------------------*/
Poly_Harmonizer* harmonizer_controller_get_harmonizer_1(HarmonizerController*  self)
{
  return self->harmonizer_1;
}

/*--------------------------------------------------------------------*/
LSTM_Harmonizer* harmonizer_controller_get_harmonizer_2(HarmonizerController*  self)
{
  return self->harmonizer_2;
}

/*--------------------------------------------------------------------*/
void harmonizer_controller_set_harmonizer(HarmonizerController*  self, int harmonizer)
{
  if(harmonizer < 1) harmonizer = 1;
  if(harmonizer > 2) harmonizer = 2;
  
  self->harmonizer = harmonizer;
}

/*--------------------------------------------------------------------*/
int harmonizer_controller_get_harmonizer(HarmonizerController*  self, int harmonizer)
{
  return self->harmonizer;
}

/*--------------------------------------------------------------------*/
int harmonizer_controller_audio_callback(void* SELF, auSample_t* buffer, int num_frames, int num_channels)
{
  HarmonizerController* self = SELF;
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

  if(self->harmonizer == 1)
    poly_harmonizer_process_audio(self->harmonizer_1, buffer, num_frames);
  else if(self->harmonizer == 2)
    lstm_harmonizer_process_audio(self->harmonizer_2, buffer, num_frames);

  return  num_frames;
}

