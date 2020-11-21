#include <Arduino.h>
#include "Interface.h"
#include "Robot_Communication.h"
#include "Solenoid.h"
#include "Eye.h"
 
#define FIRMWARE_MAJOR_VERSION 1
#define FIRMWARE_MINOR_VERSION 6

void interface_dispatch          (void* self, char* message, robot_arg_t args[], int num_args);
void interface_note_on_callback  (midi_channel_t chan, midi_pitch_t pitch, midi_velocity_t vel);
void interface_note_off_callback (midi_channel_t chan, midi_pitch_t pitch, midi_velocity_t vel);

/* 0 tap mode, 1 organ mode */
int interface_sustain_mode = 0;

/*---------------------------------------------------*/
void interface_init()
{
  midi_note_on_event_handler  = interface_note_on_callback;
  midi_note_off_event_handler = interface_note_off_callback;
  robot_init(interface_dispatch, NULL);
}

/*---------------------------------------------------*/
void interface_run_loop()
{
  uint32_t ui;
  while((ui = usb_midi_read_message()) != 0)
    {
      midi_parse((ui >>  8) & 0xFF);
      midi_parse((ui >> 16) & 0xFF);
      midi_parse((ui >> 24) & 0xFF);
    }
}

/*---------------------------------------------------*/
void interface_note_on_callback(midi_channel_t chan, midi_pitch_t pitch, midi_velocity_t vel)
{
  //if(chan == (*midi_channel) - 1)
    {
      float strength = vel / 127.0;

      switch(pitch)
        {
          case 60: /* cascade */
          case 61: /* cascade */
          case 62: /* cascade */
          case 63: /* cascade */
          case 64: /* cascade */
          case 65: /* cascade */
          case 66: /* cascade */
          case 67: /* cascade */
            if(interface_sustain_mode == 0)
              solenoid_tap_specific(pitch-60, strength);
            else //(interface_sustain_mode == 1)
              solenoid_on(pitch-60);
            break;
          default: 
            solenoid_tap(strength);
            break;
        }
    }
}

/*---------------------------------------------------*/
void interface_note_off_callback(midi_channel_t chan, midi_pitch_t pitch, midi_velocity_t vel)
{
  if(interface_sustain_mode == 1)
    {
      switch(pitch)
        {
          case 60: /* cascade */
          case 61: /* cascade */
          case 62: /* cascade */
          case 63: /* cascade */
          case 64: /* cascade */
          case 65: /* cascade */
          case 66: /* cascade */
          case 67: /* cascade */
              solenoid_off(pitch-60);
              break;
          default:
            solenoid_all_off();
            break;
        }
    }
}

/*-----------------------------------------------------*/
void interface_dispatch(void* self, char* message, robot_arg_t args[], int num_args)
{ 
  switch(robot_hash_message(message)) 
    {
      /******************************/
      case robot_hash_tap:
        if(num_args == 1)
          {
            solenoid_tap(robot_arg_to_float(&args[0]));
            robot_send_message(robot_reply_aok);
          }
         break;
      
      /******************************/
      case robot_hash_tap_specific:
        if(num_args == 2)
          {
            solenoid_tap_specific(robot_arg_to_int(&args[0]), robot_arg_to_float(&args[1]));
            robot_send_message(robot_reply_aok);
          }
         break;
      
      /******************************/
      case robot_hash_bell:
        if(num_args == 1)
          {
            solenoid_ding(robot_arg_to_float(&args[0]));
            robot_send_message(robot_reply_aok);
          }
        break;
  
      /******************************/
      case robot_hash_set_sustain_mode:
        if(num_args == 1)
          {
            unsigned mode = robot_arg_to_int(&args[0]);
            if(mode < 2)
              {
                interface_sustain_mode = mode;
                robot_send_message(robot_reply_aok);
              }
            else robot_send_message(robot_reply_error, "mode out of range");
          }
        break;

      /******************************/
      case robot_hash_note_on:
        if(num_args == 1)
          {
            solenoid_on(robot_arg_to_int(&args[0]));
            robot_send_message(robot_reply_aok);
          }
        break;
        
      /******************************/
      case robot_hash_note_off:
        if(num_args == 1)
          {
            solenoid_off(robot_arg_to_int(&args[0]));
            robot_send_message(robot_reply_aok);
          }
        break;
        
      /******************************/
      case robot_hash_play_note_for_duration:
        if(num_args == 2)
          {
            solenoid_on_for_duration(robot_arg_to_int(&args[0]), robot_arg_to_float(&args[1]));
            robot_send_message(robot_reply_aok);
          }
        break;
        
      /******************************/
      case robot_hash_all_notes_off:
        if(num_args == 1)
          {
            solenoid_all_off();
            robot_send_message(robot_reply_aok);
          }
        break;
        
      /******************************/
      case robot_hash_get_sustain_mode:
        if(num_args == 0)
          robot_send_message(robot_reply_firmware_version, interface_sustain_mode);
        break;
        
      /******************************/
      case robot_hash_get_firmware_version:
        if(num_args == 0)
           robot_send_message(robot_reply_firmware_version, FIRMWARE_MAJOR_VERSION, FIRMWARE_MINOR_VERSION); 
        break;

      /******************************/
      case robot_hash_eye_blink:
        if(num_args == 0)
          {
            eye_animate_blink();
            robot_send_message(robot_reply_aok);
          }
        break;

      /******************************/
      case robot_hash_eye_roll:
        if(num_args == 1)
          {
            eye_animate_roll(robot_arg_to_int(&args[0]));
            robot_send_message(robot_reply_aok);
          }
        break;

      /******************************/
      case robot_hash_eye_no:
        if(num_args == 2)
          {
            eye_animate_no(robot_arg_to_int(&args[0]), robot_arg_to_float(&args[1]));
            robot_send_message(robot_reply_aok);
          }
        break;
      
      /******************************/
      case robot_hash_eye_yes:
        if(num_args == 2)
          {
            eye_animate_yes(robot_arg_to_int(&args[0]), robot_arg_to_float(&args[1]));
            robot_send_message(robot_reply_aok);
          }
        break;

      /******************************/
      case robot_hash_eye_inquisitive:
        if(num_args == 0)
          {
            eye_animate_inquisitive();
            robot_send_message(robot_reply_aok);
          }
        break;

      /******************************/
      case robot_hash_eye_focused:
        if(num_args == 0)
          {
            eye_animate_focused();
            robot_send_message(robot_reply_aok);
          }
        break;
      
      /******************************/
      case robot_hash_eye_surprised:
        if(num_args == 0)
          {
            eye_animate_surprised();
            robot_send_message(robot_reply_aok);
          }
        break;

      /******************************/
      case robot_hash_eye_neutral_size:
        if(num_args == 0)
          {
            eye_animate_neutral_size();
            robot_send_message(robot_reply_aok);
          }
        break;

      /******************************/
      case robot_hash_eye_neutral_pos:
        if(num_args == 0)
          {
            eye_animate_neutral_position();
            robot_send_message(robot_reply_aok);
          }
        break;
      
      /******************************/
      default: break;
    }
}
