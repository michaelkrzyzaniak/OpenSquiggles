#include "Robot_Communication.h"
#include "MIDI_Parser.h"
#include <stdarg.h>
#include <stdio.h> //vsnprintf

/*--------------------------------------------------------*/
//should be instance var, not class var
robot_message_received_callback robot_recv_callback = NULL;
void* robot_callback_self;
void  robot_sysex_handler(midi_sysex_mfr_t mfr, char* message);

/*--------------------------------------------------------*/
#define ROBOT_SYSEX_BUFFER_SIZE 127
#define ROBOT_MIDI_MFR MIDI_MFR_OTHER

//Apple includes
#ifdef __APPLE__
#include <pthread.h>
#include <CoreMIDI/MIDIServices.h>
#include <Carbon/Carbon.h>
const   CFStringRef ROBOT_DEVICE_NAME = CFSTR(ROBOT_MIDI_DEVICE_NAME);

//Linux includes
#elif defined __linux__
#include <pthread.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <ctype.h>
#include <alsa/asoundlib.h>
#include <signal.h>

#endif

/*--------------------------------------------------------*/
#if defined __ROBOT_MIDI_HOST__
struct opaque_robot_struct
{
#if defined __APPLE__
  MIDIDeviceRef   device;
  MIDIEndpointRef source;
  MIDIEndpointRef destination;
  
  MIDIClientRef   client;
  MIDIPortRef     out_port, in_port;
  pthread_t       midi_client_thread;

#elif defined __linux__
  snd_rawmidi_t*  in_port;
  snd_rawmidi_t*  out_port;
  pthread_t       midi_read_thread;
  int             read_thread_should_continue_running;

#endif
};
#endif //__ROBOT_MIDI_HOST__

/*--------------------------------------------------------*/
#if defined __APPLE__
void    robot_connect(Robot* self);
Boolean robot_setup_midi_client(Robot* self);
void*   robot_setup_midi_client_run_loop(void* SELF);
void    robot_midi_data_recd_callback(const MIDIPacketList *pktlist, void* SELF, void *SELF2);
void    robot_midi_state_changed_callback(const MIDINotification  *message, void* SELF);
Boolean robot_check_midi_object_name(Robot* self, MIDIObjectRef midi_object);

#elif defined __linux__
int   robot_setup_midi_read_thread(Robot* self);
void* robot_read_thread_run_loop(void* SELF);
#endif //__APPLE__

/*--------------------------------------------------------*/
#if defined __APPLE__
void robot_disconnect(Robot* self)
{
  self->source = 0;
  MIDIFlushOutput(self->out_port);
}
#endif

/*--------------------------------------------------------*/
#if defined __ROBOT_MIDI_HOST__
Robot* robot_destroy(Robot* self)
{
  if(self != NULL)
    {
#if defined __APPLE__
      robot_disconnect(self);
      pthread_kill(self->midi_client_thread, SIGUSR1);

#elif defined __linux__
      self->read_thread_should_continue_running = 0;
      pthread_kill(self->midi_read_thread, SIGUSR1);
      pthread_join(self->midi_read_thread, NULL);
      snd_rawmidi_drain(self->in_port);
      snd_rawmidi_close(self->in_port);
      snd_rawmidi_drain(self->out_port);
      snd_rawmidi_close(self->out_port);
#endif
      free(self);
    }
  return (Robot*)NULL;
}
#endif// __ROBOT_MIDI_HOST__

/*--------------------------------------------------------*/
#if defined __ROBOT_MIDI_HOST__
Robot* robot_new(robot_message_received_callback callback, void* callback_self)
{
  Robot* self = (Robot*)calloc(1, sizeof(*self));
  
  if(self != NULL)
    {
      //aarg, these should be instance vars, not class vars...
      robot_init(callback, callback_self);

#if defined __APPLE__
      if(!robot_setup_midi_client(self))
        return robot_destroy(self);
#elif defined __linux__
      if(snd_rawmidi_open(&self->in_port, &self->out_port, ROBOT_MIDI_DEVICE_NAME, SND_RAWMIDI_SYNC))
        return robot_destroy(self);
      snd_rawmidi_read(self->in_port, NULL, 0); //trigger read
      if(!robot_setup_midi_read_thread(self))
        return robot_destroy(self);
#endif
    }
  return self;
}
#endif // __ROBOT_MIDI_HOST__

/*--------------------------------------------------------*/
void robot_init(robot_message_received_callback callback, void* callback_self)
{
  robot_recv_callback = callback;
  robot_callback_self = callback_self;
  midi_sysex_event_handler = robot_sysex_handler;
}

/*--------------------------------------------------------*/
#if defined __linux__
int   robot_setup_midi_read_thread(Robot* self)
{
  return !pthread_create(&self->midi_read_thread, NULL, robot_read_thread_run_loop, self);
}
#endif //__linux__

/*--------------------------------------------------------*/
#if defined __linux__
void* robot_read_thread_run_loop(void* SELF)
{
  Robot* self = (Robot*)SELF;
  uint8_t data;
  self->read_thread_should_continue_running = 1;
  
  while(self->read_thread_should_continue_running)
    {
      snd_rawmidi_read(self->in_port, &data, 1);
      //fprintf(stderr, "%02X\t%c\r\n", data);
      midi_parse(data);
    }
  return NULL;
}
#endif //__linux__

/*--------------------------------------------------------*/
#ifdef __APPLE__
void robot_connect(Robot* self)
{
  ItemCount num_sources      = MIDIGetNumberOfSources();
  MIDIEndpointRef source;
  int i;
  
  if(self->client == 0)
    {
      robot_setup_midi_client(self);
      return;
    }
  
  //find source named "Robot"
  for(i=0; i<num_sources; i++)
    {
      if((source = MIDIGetSource(i)) != 0)
        {
          if(robot_check_midi_object_name(self, source))
            {
              self->source = source;
              //fprintf(stderr, "connected\n");
              
              MIDIPortConnectSource(self->in_port, self->source, self);  
              break;
            }
        }
    }

  //get the source's parent device and destination
  if(self->source != 0)
    {
      OSStatus error = MIDIEntityGetDevice(self->source, &self->device);
      if(self->device != 0)
        self->destination = MIDIEntityGetDestination(self->device, 0);
    }
}
#endif //__APPLE__

/*--------------------------------------------------------*/
#ifdef __APPLE__
//true on success
Boolean robot_setup_midi_client(Robot* self)
{
  return !pthread_create(&self->midi_client_thread, NULL, robot_setup_midi_client_run_loop, self);
}
#endif //__APPLE__

/*--------------------------------------------------------*/
#ifdef __APPLE__
void* robot_setup_midi_client_run_loop(void* SELF)
{
  Robot* self = (Robot*)SELF;
  
  OSStatus error = noErr;
  error = MIDIClientCreate(ROBOT_DEVICE_NAME, robot_midi_state_changed_callback, self, &self->client);
  
  if(error) fprintf(stderr, "Error creating MIDI client: %d\n", error);
  
  else
    {
      error = MIDIOutputPortCreate (self->client, CFSTR("Robot Client Output Port"), &self->out_port);
      if(error) fprintf(stderr, "Error creating MIDI client output port: %d\n", error);
      else 
        {
          error = MIDIInputPortCreate(self->client, CFSTR("Robot Client Input Port"), robot_midi_data_recd_callback, self, &self->in_port);    
          if(error)
            { 
              fprintf(stderr, "Error creating MIDI client input port: %d\n", error);
              MIDIEndpointDispose(self->out_port);
            }
        }
        
      if(error)
        {
          MIDIClientDispose(self->client);
          self->client = 0;
        }
    }
  
  if(error == noErr)
    {
      robot_connect(self);
      CFRunLoopRun();
    }
  
  return NULL;
}
#endif //__APPLE__

/*--------------------------------------------------------*/
#ifdef __APPLE__
Boolean robot_check_midi_object_name(Robot* self, MIDIObjectRef midi_object)
{
  CFStringRef str;
  MIDIObjectGetStringProperty(midi_object, kMIDIPropertyName, &str);
  CFComparisonResult comparison = CFStringCompare (ROBOT_DEVICE_NAME, str, kCFCompareCaseInsensitive | kCFCompareWidthInsensitive);
  CFRelease(str);
  
  return comparison == kCFCompareEqualTo;
}
#endif //__APPLE__

/*--------------------------------------------------------*/
#ifdef __APPLE__
void robot_midi_data_recd_callback(const MIDIPacketList *packet_list, void* SELF, void *SELF2)
{
  //Robot* self = (Robot*)SELF;
  const MIDIPacket *packet = &(packet_list->packet[0]);
  int i, j;
  for(i=0; i<packet_list->numPackets; i++)
    {
      for(j=0; j<packet->length; j++)
        midi_parse(packet->data[j]);
      packet = MIDIPacketNext (packet);
    }
}
#endif //__APPLE__

/*--------------------------------------------------------*/
#ifdef __APPLE__
void robot_midi_state_changed_callback(const MIDINotification *message, void* SELF)
{
  Robot* self = (Robot*)SELF;
  
  switch(message->messageID)
    {
      case kMIDIMsgObjectAdded:
        if((self->source == 0) && (((MIDIObjectAddRemoveNotification*)message)->childType == kMIDIObjectType_Source))
          robot_connect(self);
        break;
        
      case kMIDIMsgObjectRemoved:
        if((self->source != 0) && (((MIDIObjectAddRemoveNotification*)message)->childType == kMIDIObjectType_Source))
          if(robot_check_midi_object_name(self, ((MIDIObjectAddRemoveNotification*)message)->child))
            robot_disconnect(self);
        break;
        
      case kMIDIMsgSetupChanged:
        break;
      case kMIDIMsgPropertyChanged :
        break;
      case kMIDIMsgThruConnectionsChanged:
        break;
      case kMIDIMsgSerialPortOwnerChanged:
        break;
      case kMIDIMsgIOError:
        break;
      default: break;
    }
}
#endif //__APPLE__

/*--------------------------------------------------------*/
#if defined __ROBOT_MIDI_HOST__
void robot_send_raw_midi(Robot* self, uint8_t* midi_bytes, int num_bytes)
{
#if defined __APPLE__
	MIDIPacketList packet_list;
	MIDIPacket* current_packet;
	current_packet = MIDIPacketListInit(&packet_list);	
	current_packet = MIDIPacketListAdd(&packet_list, sizeof(packet_list), current_packet, 0 /*mach_absolute_time()*/, num_bytes, midi_bytes);
  MIDISend(self->out_port, self->destination, &packet_list);
  
#elif defined __linux__
  snd_rawmidi_write(self->out_port, midi_bytes, num_bytes);
  //snd_rawmidi_drain(self->out_port);

#endif
}
#endif //defined __ROBOT_MIDI_HOST__

/*--------------------------------------------------------*/
#if defined __ROBOT_MIDI_HOST__
void robot_send_message(Robot* self, const char *fmt, ...)
{
#if defined __APPLE__
  if(self->source != 0)
#endif
    {
#else  //MIDI CLIENT
void robot_send_message(const char *fmt, ...)
{

#endif //SHARED CODE
      uint8_t  buffer[ROBOT_SYSEX_BUFFER_SIZE];
      uint8_t* b = buffer;

      va_list args;
      va_start(args, fmt);

      *b++ = MIDI_STATUS_SYSTEM_EXCLUSIVE;
      *b++ = ROBOT_MIDI_MFR;

      b += vsnprintf((char*)b, ROBOT_SYSEX_BUFFER_SIZE-2, fmt, args);

      if(b-buffer >= ROBOT_SYSEX_BUFFER_SIZE)
        b = buffer + ROBOT_SYSEX_BUFFER_SIZE - 1;
/*
      int i;
      fprintf(stderr, "%s\r\n", buffer+2);
      int len = b - buffer;
      for(i=0; i<len; i++)
        fprintf(stderr, "%02X ", buffer[i]);
      fprintf(stderr, "\r\n ");
*/
      //replace '\0' with EOX
      *b++ = MIDI_STATUS_END_OF_EXCLUSIVE;
      
#ifdef __ROBOT_MIDI_HOST__
      robot_send_raw_midi(self, buffer, b - buffer);
    }
    
#else   //MIDI CLIENT
      usb_midi_send_sysex_buffer_has_term(buffer, b-buffer);

#endif //SHARED CODE
}

/*---------------------------------------------------*/
void robot_sysex_handler(midi_sysex_mfr_t mfr, char* message)
{
  //fprintf(stderr, "SYSEX\tmfr:%i\tmessage:%s\t\n", mfr, message); 
  int MAX_NUM_ARGS = 8;
  robot_arg_t args[MAX_NUM_ARGS], *a = args;
  
  if(mfr != ROBOT_MIDI_MFR) return;
  
  while(1)
    {
      if((a-args) < MAX_NUM_ARGS)
        {
          int contains_period = 0;
          int should_break    = 0;
          char* end;
          
          //strip whitespace chars
          while((*message <= ' ') && (*message != '\0'))
            message++;
                    
          if(*message == '\0')
            break;
          
           end = message;
           
          //find next space and replace with null
          while((*end != '\0') && (*end != ' '))
            if(*end++ == '.')
              contains_period = 1;

          if(*end == '\0') should_break = 1;
          else *end = '\0';
          
          //next argrument is a number
          if(((*message >= '0') && (*message <= '9')) || (*message == '+') || (*message == '-'))
            {            
              if(contains_period) //it is a float
                {
                  a->type = 'f';
                  a->value.f = (float)atof(message);
                  a++;
                }
              else // it is an int
                {
                  a->type = 'i';
                  a->value.i = (int32_t)atol(message);
                  a++;
                }
            }

          //next argument is a string
          else if((*message >= '!') && (*message <= '~'))
            {
              a->type = 's';
              a->value.s = message;
              a++;          
            }

          if(should_break) break;
          else(message = end+1);
        }
      else //fmt string is full
        break;
    }
    
    if(args[0].type == 's')
      robot_recv_callback(robot_callback_self, args[0].value.s, args+1, a-args-1);
}

/*-----------------------------------------------------*/
float robot_arg_to_float(robot_arg_t* arg)
{
  float result = 0;
  switch(arg->type)
    {
      case 'f': 
        result = arg->value.f;
        break;
      case 'i':
        result = arg->value.i;
        break;
      case 's':
        result = (float)atof(arg->value.s);
        break;
      default: 
        break;
    }
    return result;
}

/*-----------------------------------------------------*/
int32_t robot_arg_to_int(robot_arg_t* arg)
{
  int32_t result = 0;
  switch(arg->type)
    {
      case 'f': 
        result = arg->value.f;
        break;
      case 'i':
        result = arg->value.i;
        break;
      case 's':
        result = (int32_t)atol(arg->value.s);
        break;
      default: 
        break;
    }
  return result;  
}

/*-----------------------------------------------------*/
uint32_t robot_hash_message(char* message)
{
  uint32_t hash = 5381;
  while (*message != '\0')
    hash = hash * 33 ^ (int)*message++;
  return hash;
}

/*-----------------------------------------------------*/
void robot_debug_print_message(char* message, robot_arg_t args[], int num_args)
{
#ifdef __APPLE__
  int i;
  fprintf(stderr, "recd: %s(", message);
  
  for(i=0; i<num_args; i++)
    { 
      if(i != 0)
        putchar(' ');
      printf("<%c>", args[i].type);
      switch(args[i].type)
        {
          case 'f':
            printf("%f", args[i].value.f); break;
          case 'i':
            printf("%i", args[i].value.i); break;
          case 's':
            printf("%s", args[i].value.s); break;
        }
    }
  printf(");\r\n");
#endif// __APPLE__
}
