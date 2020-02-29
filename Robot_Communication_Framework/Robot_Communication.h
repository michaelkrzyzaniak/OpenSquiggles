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
  robot_hash_tap_specific                = 642191216,
  robot_hash_bell                        = 100947277,
  robot_hash_eye_blink                   = 3331026536,
  robot_hash_eye_roll                    = 101520471,
  robot_hash_eye_no                      = 193346379,
  robot_hash_eye_yes                     = 2085481541,
  robot_hash_eye_inquisitive             = 3189208244,
  robot_hash_eye_focused                 = 3273344903,
  robot_hash_eye_surprised               = 1264673671,
  robot_hash_eye_neutral_size            = 3929165797,
  robot_hash_eye_neutral_pos             = 1030114188,
  robot_hash_get_firmware_version        = 1616850098,
  robot_hash_reply_firmware_version      = 857564406,
  robot_hash_aok                         = 2085472399,
  robot_hash_error                       = 3342388946,
}robot_message_hash_t;

/*--------------------------------------------------------*/
#define robot_cmd_tap                         "/tap %f"             //strength, 0.0~1.0
#define robot_cmd_tap_specific                "/tap_specific %i %f" //index 0-7 strength, 0.0~1.0
#define robot_cmd_bell                        "/bell %f"            //strength, 0.0~1.0
#define robot_cmd_get_firmware_version        "/get_firmware"

#define robot_cmd_eye_blink                   "/blink"
#define robot_cmd_eye_roll                    "/roll %i"            //depth
#define robot_cmd_eye_no                      "/no %i %f"           //depth, speed
#define robot_cmd_eye_yes                     "/yes %i %f"          //depth, speed
#define robot_cmd_eye_inquisitive             "/inquisitive"
#define robot_cmd_eye_focused                 "/focused"
#define robot_cmd_eye_surprised               "/surprised"
#define robot_cmd_eye_neutral_size            "/neutral_size"
#define robot_cmd_eye_neutral_position        "/neutral_pos"

#define robot_reply_aok                       "/aok"
#define robot_reply_error                     "/error %s"
#define robot_reply_firmware_version          "/reply_firmware %i %i"

/*---------------------------------------------------*/
typedef void  (*robot_message_received_callback)(void* self, char* message, robot_arg_t args[], int num_args);

/*--------------------------------------------------------*/

#if (defined __APPLE__) || (defined __linux__)
#define __ROBOT_MIDI_HOST__ 1
#endif

#if defined  __APPLE__
#define ROBOT_MIDI_DEVICE_NAME "Dr Squiggles"

#elif defined  __linux__
//cat /proc/asound/cards for a list of cards -- I guess Linux dosen't like the space in the name?
#define ROBOT_MIDI_DEVICE_NAME "hw:Squiggles"
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
