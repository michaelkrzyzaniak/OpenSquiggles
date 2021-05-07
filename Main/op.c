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
//gcc op.c core/*.c ../Robot_Communication_Framework/*.c ../Beat-and-Tempo-Tracking/src/*.c Rhythm_Generators/*.c extras/*.c -framework CoreMidi -framework Carbon -framework AudioToolbox -O2 -o op

//Linux compile with:
//sudo apt-get install libasound2-dev
//gcc op.c core/*.c ../Robot_Communication_Framework/*.c ../Beat-and-Tempo-Tracking/src/*.c Rhythm_Generators/*.c extras/*.c -lasound -lm -lpthread -lrt -O2 -o op

#define __OP_VERSION__ "0.1"

#define clip(in, val, min, max) in = (val < min) ? min : (val > max) ? max : val;

void i_hate_canonical_input_processing(void);
void make_stdin_cannonical_again();

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "extras/Network.h"
#include "extras/OSC.h"
#include "extras/Params.h"
#include "../Robot_Communication_Framework/Robot_Communication.h" //calloc
#include "../Robot_Communication_Framework/MIDI_Parser.h" //calloc

#define  OSC_BUFFER_SIZE 512
#define  OSC_VALUES_BUFFER_SIZE 64
#define  OSC_SEND_PORT   9000
#define  OSC_RECV_PORT   9001

#define PARAMS_PATH ".squiggles_notes/squiggles_notes.xml" //it will go in home directory
#define NUM_SOLENOIDS 8

/*--------------------------------------------------------------------*/
typedef struct opaque_osc_responder_struct
{
  Network* net;
  char* osc_send_buffer;
  char* osc_recv_buffer;
  oscValue_t* osc_values_buffer;
  pthread_t osc_recv_thread;
  //pthread_mutex_t onset_buffer_mutex;
  
  Params* params;
  int notes[NUM_SOLENOIDS];
  Robot* robot; //teensy mcu
  
}OSC_Responder;

/*--------------------------------------------------------------------*/
void* osc_responder_recv_thread_run_loop(void* self);
void  osc_responder_robot_message_received_callback(void* SELF, char* message, robot_arg_t args[], int num_args);
void  osc_responder_init_midi_note_params(OSC_Responder* self);
OSC_Responder* osc_responder_destroy (OSC_Responder* self);
int osc_responder_midi_to_solenoid_number(OSC_Responder* self, int midi_note);

void osc_responder_midi_note_on_event_handler(midi_channel_t chan, midi_pitch_t note, midi_velocity_t vel);
void osc_responder_midi_note_off_event_handler(midi_channel_t chan, midi_pitch_t note, midi_velocity_t vel);
void osc_responder_midi_mode_change_event_handler(midi_channel_t  chan, midi_mode_t mode, uint8_t arg);

OSC_Responder* responder; //todo fix this --should not be global variable, but MIDI Parser needs void* SELF in callbacks

/*--------------------------------------------------------------------*/
OSC_Responder* osc_responder_new()
{
  OSC_Responder* self = (OSC_Responder*) calloc(1, sizeof(*self));
  
  if(self != NULL)
    {
      self->osc_send_buffer = calloc(1, OSC_BUFFER_SIZE);
      if(!self->osc_send_buffer) return osc_responder_destroy(self);
      self->osc_recv_buffer = calloc(1, OSC_BUFFER_SIZE);
      if(!self->osc_recv_buffer) return osc_responder_destroy(self);
      self->osc_values_buffer = calloc(sizeof(*self->osc_values_buffer), OSC_VALUES_BUFFER_SIZE);
      if(!self->osc_values_buffer) return osc_responder_destroy(self);

      self->net  = net_new();
      if(!self->net)
        return osc_responder_destroy(self);
      if(!net_udp_connect (self->net, OSC_RECV_PORT))
        return osc_responder_destroy(self);

      //if(pthread_mutex_init(&self->onset_buffer_mutex, NULL) != 0)
        //return osc_responder_destroy(self);
    
      self->robot = robot_new(osc_responder_robot_message_received_callback, NULL);
      
      if(self->robot == NULL)
        return osc_responder_destroy(self);
      
      const char* home = getenv("HOME");
      int n = strlen(home) + strlen(PARAMS_PATH) + 2;
      char path[n];
      memset(path, 0, n);
      strcpy(path, home);
      strcat(path, "/");
      strcat(path, PARAMS_PATH);
      self->params = params_new(path);
      if(self->params == NULL)
        return osc_responder_destroy(self);
        
      osc_responder_init_midi_note_params(self);
      
      midi_note_on_event_handler = osc_responder_midi_note_on_event_handler;
      midi_note_off_event_handler = osc_responder_midi_note_off_event_handler;
      midi_mode_change_event_handler = osc_responder_midi_mode_change_event_handler;
    
      int error = pthread_create(&self->osc_recv_thread, NULL, osc_responder_recv_thread_run_loop, self);
      if(error != 0)
        return osc_responder_destroy(self);
    }
  
  return (OSC_Responder*)self;
}

/*--------------------------------------------------------------------*/
OSC_Responder*      osc_responder_destroy (OSC_Responder* self)
{
  if(self != NULL)
    {
      /* free any malloc-ed instance variables here */
    
      fprintf(stderr, "Here Free 1\r\n");
      if(self->osc_recv_thread != 0)
        {
          pthread_cancel(self->osc_recv_thread);
          
          fprintf(stderr, "Here Free 1.5\r\n");
          pthread_join(self->osc_recv_thread, NULL);
        }
      
      fprintf(stderr, "Here Free 2\r\n");
      if(self->osc_send_buffer != NULL)
        free(self->osc_send_buffer);
        
      fprintf(stderr, "Here Free 2\r\n");
      if(self->osc_recv_buffer != NULL)
        free(self->osc_recv_buffer);
        
      fprintf(stderr, "Here Free 3\r\n");
      if(self->net != NULL)
        net_disconnect(self->net);
        
      fprintf(stderr, "Here Free 4\r\n");
      net_destroy(self->net);
      
      fprintf(stderr, "Here Free 5\r\n");
      robot_destroy(self->robot);
      
      fprintf(stderr, "Here Free 6\r\n");
      params_destroy(self->params);
      
      fprintf(stderr, "Here Free 7\r\n");
      free(self);
    }
  return (OSC_Responder*) NULL;
}

/*--------------------------------------------------------------------*/
void* osc_responder_recv_thread_run_loop(void* SELF)
{
  OSC_Responder* self = (OSC_Responder*)SELF;
  char senders_address[16];
  char *osc_address, *osc_type_tag;
  
  for(;;)
  {
    int num_valid_bytes = net_udp_receive(self->net, self->osc_recv_buffer, OSC_BUFFER_SIZE, senders_address);
    //fprintf(stderr, "-->  net_udp_receive %i bytes\r\n", num_valid_bytes);
    if(num_valid_bytes < 0)
      continue; //return NULL ?
  
    int num_osc_values = oscParse(self->osc_recv_buffer, num_valid_bytes, &osc_address, &osc_type_tag, self->osc_values_buffer, OSC_VALUES_BUFFER_SIZE);
    //fprintf(stderr, "-->  osc_parse %s %s [%i values]\r\n", osc_address, osc_type_tag, num_osc_values);
    if(num_osc_values < 0)
        continue;
    
    uint32_t address_hash = oscHash((unsigned char*)osc_address);
    if(address_hash == 100987747) // '/midi'
      {
        int i;
        for(i=0; i<num_osc_values; i++)
          {
            int stream = oscValueAsInt(self->osc_values_buffer[i], osc_type_tag[i]);
            //fprintf(stderr, "     --> %02X\r\n", (unsigned char)stream);
            if ((stream >= 0) && (stream <= 0xFF))
              midi_parse((uint8_t)stream);
          }
      }
  }
}

/*--------------------------------------------------------------------*/
void  osc_responder_robot_message_received_callback(void* IGNORED_SELF, char* message, robot_arg_t args[], int num_args)
{
  //ignore incoming messages, PI dosn't send them;
}

/*--------------------------------------------------------------------*/
void osc_responder_init_midi_note_params(OSC_Responder* self)
{
  //initalize only if it dosen't already exist
  params_init_int(self->params, "solenoid_0_note", 60);
  params_init_int(self->params, "solenoid_1_note", 62);
  params_init_int(self->params, "solenoid_2_note", 64);
  params_init_int(self->params, "solenoid_3_note", 65);
  params_init_int(self->params, "solenoid_4_note", 67);
  params_init_int(self->params, "solenoid_5_note", 69);
  params_init_int(self->params, "solenoid_6_note", 71);
  params_init_int(self->params, "solenoid_7_note", 72);
  
  self->notes[0] = params_get_int(self->params, "solenoid_0_note", -1);
  self->notes[1] = params_get_int(self->params, "solenoid_1_note", -1);
  self->notes[2] = params_get_int(self->params, "solenoid_2_note", -1);
  self->notes[3] = params_get_int(self->params, "solenoid_3_note", -1);
  self->notes[4] = params_get_int(self->params, "solenoid_4_note", -1);
  self->notes[5] = params_get_int(self->params, "solenoid_5_note", -1);
  self->notes[6] = params_get_int(self->params, "solenoid_6_note", -1);
  self->notes[7] = params_get_int(self->params, "solenoid_7_note", -1);
}

/*--------------------------------------------------------------------*/
int osc_responder_midi_to_solenoid_number(OSC_Responder* self, int midi_note)
{
  int result  = -1;
  int i;
  for(i=0; i<NUM_SOLENOIDS; i++)
    if(self->notes[i] == midi_note)
      {
        result = i;
        break;
      }
  return result;
}

/*--------------------------------------------------------------------*/
void osc_responder_midi_note_on_event_handler(midi_channel_t chan, midi_pitch_t note, midi_velocity_t vel)
{
  int solenoid = osc_responder_midi_to_solenoid_number(/*self*/responder, note);
  if(solenoid >= 0)
    {
      robot_send_message(/*self*/responder->robot, robot_cmd_note_on, solenoid);
      robot_send_message(/*self*/responder->robot, robot_cmd_eye_blink);
      fprintf(stderr, "NOTE ON -- chan: %u\tnote: %u\tvel:%u\r\n", chan, note, vel);
    }
}

/*--------------------------------------------------------------------*/
void osc_responder_midi_note_off_event_handler(midi_channel_t chan, midi_pitch_t note, midi_velocity_t vel)
{
  int solenoid = osc_responder_midi_to_solenoid_number(/*self*/responder, note);
  if(solenoid >= 0)
    robot_send_message(/*self*/responder->robot, robot_cmd_note_off, solenoid);
    //fprintf(stderr, "NOTE OFF -- chan: %u\tnote: %u\tvel:%u\r\n", chan, note, vel);
}

/*--------------------------------------------------------------------*/
void osc_responder_midi_mode_change_event_handler(midi_channel_t  chan, midi_mode_t mode, uint8_t arg)
{
 //0xB0 0x78 0,00
  if(mode == MIDI_MODE_ALL_SOUND_OFF)
    {
      robot_send_message(/*self*/responder->robot, robot_cmd_all_notes_off);
      fprintf(stderr, "ALL NOTES OFF -- chan: %u\tmode: %u\targ:%u\r\n", chan, mode, arg);
    }
}

/*--------------------------------------------------------------------*/
int main(void)
{
  //i_hate_canonical_input_processing();

  //defined globally because reasons
  responder = osc_responder_new();
  if(responder == NULL) {perror("Yikes!"); return(-1);}
 
  robot_send_message(responder->robot, robot_cmd_set_sustain_mode, 1);

  for(;;)
    if(getchar() == 'q')
      break;
 
  robot_send_message(responder->robot, robot_cmd_set_sustain_mode, 0);
  responder = osc_responder_destroy(responder);
  
  //make_stdin_cannonical_again();
}

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
struct termios old_terminal_attributes;

void i_hate_canonical_input_processing(void)
{
  int error;
  struct termios new_terminal_attributes;
  
  int fd = fcntl(STDIN_FILENO,  F_DUPFD, 0);
  
  error = tcgetattr(fd, &(old_terminal_attributes));
  if(error == -1) {  fprintf(stderr, "Error getting serial terminal attributes\r\n"); return;}
  
  new_terminal_attributes = old_terminal_attributes;
  
  cfmakeraw(&new_terminal_attributes);
  
  error = tcsetattr(fd, TCSANOW, &new_terminal_attributes);
  if(error == -1) {  fprintf(stderr,  "Error setting serial attributes\r\n"); return; }
}

/*--------------------------------------------------------------------*/
void make_stdin_cannonical_again()
{
  int fd = fcntl(STDIN_FILENO,  F_DUPFD, 0);
  
  if (tcsetattr(fd, TCSANOW, &old_terminal_attributes) == -1)
    fprintf(stderr,  "Error setting serial attributes\r\n");
}
