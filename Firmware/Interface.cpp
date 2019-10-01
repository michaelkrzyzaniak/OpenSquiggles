#include <Arduino.h>
#include "Interface.h"
#include "Robot_Communication.h"
#include "Solenoid.h"

void interface_dispatch          (void* self, char* message, robot_arg_t args[], int num_args);
void interface_note_on_callback  (midi_channel_t chan, midi_pitch_t pitch, midi_velocity_t vel);
void interface_note_off_callback (midi_channel_t chan, midi_pitch_t pitch, midi_velocity_t vel);
void interface_send_aok          ();
void interface_send_error        (const char* error);

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
  /* robot_communication depends on this too */
  while(usbMIDI.available())
    midi_parse(usbMIDI.read_raw());
}

/*---------------------------------------------------*/
void interface_send_aok()
{ 
  robot_send_message(robot_reply_aok);
}

/*---------------------------------------------------*/
void interface_send_error(const char* error)
{ 
  robot_send_message(robot_reply_error, error);
}

/*---------------------------------------------------*/
void interface_note_on_callback(midi_channel_t chan, midi_pitch_t pitch, midi_velocity_t vel)
{
  //robot_send_message("here:%i %i %i", chan, pitch, vel);
  //uint8_t* midi_channel = (uint8_t*) (MMAP_ADDR_CACHE_START + MMAP_ADDR_MIDI_CHANNEL);
  
  //if(chan == (*midi_channel) - 1)
    {
      float spd = vel / 127.0;
      
      switch(pitch)
        {
          
          default: 
            solenoid_tap(spd);
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
            interface_send_aok();
          }
         break;
      case robot_hash_bell:
        if(num_args == 1)
          {
            solenoid_ding(robot_arg_to_float(&args[0]));
            interface_send_aok();
          }
        break;
        
      /*---------------------------------------------------*/
      default: break;
    }
}
