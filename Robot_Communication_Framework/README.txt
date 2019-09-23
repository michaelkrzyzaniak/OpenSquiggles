/*
  This is a C language framework for communicating with Kiki.
  It is used by all software in this project. Below
  is a sample app that shows how to send and receive messages.
  This library uses something similar to OSC, and 
  sends these messages as MIDI system exclusive messages.
  The robot also responds to a few note-on messages
  for speed.

  gcc *.c -framework CoreMidi -framework Carbon
*/

#include "Kiki_Communication.h"
#include <stdio.h>
#include <stdlib.h>

/*--------------------------------------------------------*/
void message_recd(void* self, char* message, kiki_arg_t args[], int num_args)
{
  kiki_debug_print_message(message, args, num_args);
  
  switch(kiki_hash_message(message))
    {
      case kiki_hash_mmap_addr:
        fprintf(stderr, "kiki just replied to a mmap request\r\n");
        break;
      
      default: break;
    }
}

/*--------------------------------------------------------*/
int main(void)
{
  Kiki* kiki = kiki_new(message_recd, NULL);
  
  if(kiki == NULL)
    {fprintf(stderr, "unable to create kiki object\n"); exit(-1);}
  
  uint8_t midi_bytes[3] = {0x80, 0x64, 0x64};
  
  while(1)
    {
      sleep(2);
      
      //some different messages you can send the robot
      kiki_send_message(kiki, kiki_cmd_bass, ARM_INDEX_LEFT, 0.8 /*speed*/);
      //kiki_send_message(kiki, kiki_cmd_mmap_get, MMAP_ADDR_L_SOLENOID_MIN_EJECT_DURATION)
      //kiki_send_raw_midi(kiki, midi_bytes, 3);
    }
  
  return 0;
}
