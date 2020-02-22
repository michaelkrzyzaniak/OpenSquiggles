#include <Arduino.h>
#include "Interface.h"
#include "Robot_Communication.h"
#include "Solenoid.h"
#include "Eye.h"

#define FIRMWARE_MAJOR_VERSION 1
#define FIRMWARE_MINOR_VERSION 2

void interface_dispatch          (void* self, char* message, robot_arg_t args[], int num_args);
void interface_note_on_callback  (midi_channel_t chan, midi_pitch_t pitch, midi_velocity_t vel);
void interface_note_off_callback (midi_channel_t chan, midi_pitch_t pitch, midi_velocity_t vel);

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
  //robot_send_message("here:%i %i %i", chan, pitch, vel);
  
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
              solenoid_tap_specific((pitch-60), strength); 
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

}

/*-----------------------------------------------------*/
void interface_dispatch(void* self, char* message, robot_arg_t args[], int num_args)
{ 
  switch(robot_hash_message(message)) 
    {
      /*---------------------------------------------------*/  
      case robot_hash_tap:
        if(num_args == 1)
          {
            solenoid_tap(robot_arg_to_float(&args[0]));
            robot_send_message(robot_reply_aok);
          }
         break;
      case robot_hash_tap_specific:
        if(num_args == 2)
          {
            solenoid_tap_specific(robot_arg_to_int(&args[0]), robot_arg_to_float(&args[1]));
            robot_send_message(robot_reply_aok);
          }
         break;
      case robot_hash_bell:
        if(num_args == 1)
          {
            solenoid_ding(robot_arg_to_float(&args[0]));
            eye_animate_blink();
            robot_send_message(robot_reply_aok);
          }
        break;
      case robot_hash_get_firmware_version:
        if(num_args == 0)
        {
          robot_send_message(robot_reply_firmware_version, FIRMWARE_MAJOR_VERSION, FIRMWARE_MINOR_VERSION);
          usbMIDI.sendNoteOn(69, 100, 1);
        }
        break;
        
      /*---------------------------------------------------*/
      default: break;
    }
}
