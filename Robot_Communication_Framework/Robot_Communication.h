#ifndef ROBOT_COMMUNICATION
#define ROBOT_COMMUNICATION 1

#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

#include <stdint.h>
#include "MIDI_Parser.h"

/*---------------------------------------------------*/
typedef struct robot_arg_struct
{
  union robot_arg_value_union
    {
      float   f;
      int32_t i;
      char*   s;
    }value;

  char type;
}robot_arg_t;

/*---------------------------------------------------*/
typedef enum robot_message_hash_enum
{
  robot_hash_tap                         = 2085486991,
  robot_hash_bell                        = 100947277,
  robot_hash_aok                         = 2085472399,
  robot_hash_error                       = 3342388946,
    
}robot_message_hash_t;

/*--------------------------------------------------------*/
#define robot_cmd_tap                         "/tap %f"        //strength, 0.0~1.0
#define robot_cmd_bell                        "/bell %f"       //strength, 0.0~1.0
#define robot_reply_aok                       "/aok"
#define robot_reply_error                     "/error %s"

/*---------------------------------------------------*/
typedef void  (*robot_message_received_callback)(void* self, char* message, robot_arg_t args[], int num_args);

/*--------------------------------------------------------*/

#if (defined __APPLE__) || (defined __linux__)
#define __ROBOT_MIDI_HOST__ 1
#endif

#if defined  __APPLE__
#define ROBOT_MIDI_DEVICE_NAME "Dr Squiggles"

#elif defined  __linux__
#define ROBOT_MIDI_DEVICE_NAME "hw:1,0,0"
#endif

#if defined __ROBOT_MIDI_HOST__
typedef   struct opaque_robot_struct Robot;
Robot*    robot_new                (robot_message_received_callback callback, void* callback_self);
Robot*    robot_destroy            (Robot* self);
void      robot_send_message       (Robot* self, const char *message, /*args*/...);
void      robot_send_raw_midi      (Robot* self, uint8_t* midi_bytes, int num_bytes);

#else //!__ROBOT_MIDI_HOST__,
void     robot_send_message       (const char *message, /*args*/...);

#endif // __ROBOT_MIDI_HOST__

//SHARED CODE
void     robot_init               (robot_message_received_callback callback, void* callback_self);
float    robot_arg_to_float       (robot_arg_t* arg);
int32_t  robot_arg_to_int         (robot_arg_t* arg);

uint32_t robot_hash_message       (char* message);
void     robot_debug_print_message(char* message, robot_arg_t args[], int num_args);

#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif //ROBOT_COMMUNICATION
