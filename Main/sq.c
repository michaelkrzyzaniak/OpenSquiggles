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
//gcc sq.c core/*.c ../Robot_Communication_Framework/*.c ../Beat-and-Tempo-Tracking/src/*.c Rhythm_Generators/*.c extras/*.c -framework CoreMidi -framework Carbon -framework AudioToolbox -O2 -o sq

//Linux compile with:
//sudo apt-get install libasound2-dev
//gcc sq.c core/*.c ../Robot_Communication_Framework/*.c ../Beat-and-Tempo-Tracking/src/*.c Rhythm_Generators/*.c extras/*.c -lasound -lm -lpthread -lrt -O2 -o sq

#define __SQ_VERSION__ "1.26"

#include "core/Microphone.h"
#include <string.h> //strcmp

#define clip(in, val, min, max) in = (val < min) ? min : (val > max) ? max : val;

void i_hate_canonical_input_processing(void);
void make_stdin_cannonical_again();

int enter_rhythm_submenu(Microphone* mic, int indent_level);

BTT* global_beat_tracker;


/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
//todo: make this into its own class in a separate file
#define  OSC_BUFFER_SIZE 512
#define  OSC_VALUES_BUFFER_SIZE 64
#define  OSC_SEND_PORT   7001
#define  OSC_RECV_PORT   7000

#include <pthread.h>
#include "extras/Network.h"
#include "extras/OSC.h"

void*           main_recv_thread_run_loop(void* SELF);
Network*        net;
oscValue_t*     osc_values_buffer;
char*           osc_recv_buffer;
pthread_t       osc_recv_thread;

int main_init_osc_communication(void* SELF)
{
  //osc_send_buffer = calloc(1, OSC_BUFFER_SIZE);
  //if(!osc_send_buffer) return 0;
  osc_recv_buffer = calloc(1, OSC_BUFFER_SIZE);
  if(!osc_recv_buffer) return 0;
  osc_values_buffer = calloc(sizeof(*osc_values_buffer), OSC_VALUES_BUFFER_SIZE);
  if(!osc_values_buffer) return 0;

  net  = net_new();
  if(!net) return 0;
  if(!net_udp_connect(net, OSC_RECV_PORT))
    return 0;
    
  int error = pthread_create(&osc_recv_thread, NULL, main_recv_thread_run_loop, SELF);
  if(error != 0)
    return 0;

  return 1;
}

/*--------------------------------------------------------------------*/
void*  main_recv_thread_run_loop(void* SELF)
{
  Microphone*  mic   = SELF;
  BTT*         btt   = mic_get_btt(mic);
  Robot*       robot = mic_get_robot(mic);
  
  char senders_address[16];
  char *osc_address, *osc_type_tag;
  
  for(;;)
  {
    int num_valid_bytes = net_udp_receive(net, osc_recv_buffer, OSC_BUFFER_SIZE, senders_address);
    if(num_valid_bytes < 0)
      continue; //return NULL ?
    int num_osc_values = oscParse(osc_recv_buffer, num_valid_bytes, &osc_address, &osc_type_tag, osc_values_buffer, OSC_VALUES_BUFFER_SIZE);
  
    uint32_t address_hash = oscHash((unsigned char*)osc_address);
    if(address_hash == 4009690692) // '/play_tapping'
      {
        btt_set_tracking_mode(btt, BTT_ONSET_AND_TEMPO_AND_BEAT_TRACKING);
        btt_init_tempo(btt, 96 /*0 to clear tempo*/);
        auPlay((Audio*)mic);
      }
    else if(address_hash == 79269778) // '/pause_tapping'
      {
         auPause((Audio*)mic);
         //btt_clear(btt);
      }
  }
}
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/




typedef void   (*double_setter)(void* self, double val);
typedef double (*double_getter)(void* self);
typedef void   (*int_setter)   (void* self, int val);
typedef void*  (*int_setter_o) (void* self, int val);
typedef int    (*int_getter)   (void* self);
typedef int    (*enter_funct)  (void* self, int val);
typedef void   (*funct)        (void);

typedef struct parameter_struct
{
  funct  set;
  funct  get;
  funct  enter;
  void*  self;
  char   type;
  double init;
  double increment;
  char   name[128];
}param_t;

/*--------------------------------------------------------------------*/
void indent(int level)
{
  fprintf(stderr, "\033[F\r\033[2K");
  int i;
  for(i=0; i<level; i++)
    fprintf(stderr, "\t");
}

/*--------------------------------------------------------------------*/
int cycle_through_paramaters_and_take_get_input(const char* object_name, param_t* params, int num_params, const char* submenu_message, int indent_level)
{
  int param_index = 0;
  int exit_code = 1;
  char c;
  double val, increment;
  
  fprintf(stderr, "\r\n\r\n");
  indent(indent_level);
  fprintf(stderr, "%s:\r\n\r\n", submenu_message);
  
 
  /* simulate left arrow press to trigger display of first paramater */
  ungetc('D', stdin);
  
  for(;;)
    {
      c = getchar();
      increment = 0;
      param_t p = params[param_index];
      switch(c)
        {
          case 'C':
            ++param_index;
            if(param_index >= num_params) param_index = num_params - 1;
            p = params[param_index];
            break;
          case 'D':
            --param_index;
            if(param_index < 0) param_index = 0;
            p = params[param_index];
            break;
          case 'B':
            increment = -p.increment;
            break;
          case 'A':
            increment = p.increment;
            break;

          case 'd':
            if(p.type == 'i')
              ((int_setter) p.set)(p.self, p.init);
            else if(p.type == 'd')
              ((double_setter) p.set)(p.self, p.init);
            break;

          case '\r':
          //case '\\':
            if((p.enter != NULL))
              exit_code = ((enter_funct)p.enter)(p.self, indent_level+1);
              if(exit_code == -1)
                goto out;
            continue;


          case 'c':
            btt_set_tracking_mode(global_beat_tracker, BTT_COUNT_IN_TRACKING);
            continue;
          case 'l':
            btt_set_tracking_mode(global_beat_tracker, BTT_TEMPO_LOCKED_BEAT_TRACKING);
            continue;
          case 'L':
            btt_set_tracking_mode(global_beat_tracker, BTT_ONSET_AND_TEMPO_AND_BEAT_TRACKING);
            continue;


          case 'Q':
            exit_code = -1;
            /* cascade */
          case 'q':
            goto out;
            break;

          default: continue;
        }


      if(p.type == 'i')
        {
          val = ((int_getter) p.get)(p.self);
          ((int_setter) p.set)(p.self, val + increment);
          val = ((int_getter) p.get)(p.self);
        
          indent(indent_level);
          if(p.get == (funct)btt_get_tracking_mode)
            fprintf(stderr, " -- %s(%s, %s);\r\n", p.name, object_name, btt_get_tracking_mode_string(p.self));
          else
            fprintf(stderr, " -- %s(%s, %lf);\r\n", p.name, object_name, val);
        }
      else if(p.type == 'd')
        {
          val = ((double_getter) p.get)(p.self);
          ((double_setter) p.set)(p.self, val + increment);
           val = ((double_getter) p.get)(p.self);
          indent(indent_level);
          fprintf(stderr, " -- %s(%s, %lf);\r\n", p.name, object_name, val);
        }
      else if(p.type == 'o')
        {
          val = ((int_getter) p.get)(p.self);
          void* r = ((int_setter_o) p.set)(p.self, val + increment);
          const char* name = "NULL";
          if(r !=  NULL) name = rhythm_get_name(r);
          indent(indent_level);
          fprintf(stderr, " -- %s(%s, %s);\r\n", p.name, object_name, name);
        }
      else if(p.type == 'e')
        {
          indent(indent_level);
          fprintf(stderr, " -- [ENTER] %s\r\n", p.name);
        }

    }
  
 out:
  fprintf(stderr, "\033[F\r\033[2K\033[F\r\033[2K\033[F\r\033[2K");
  return exit_code;
}

/*--------------------------------------------------------------------*/
int enter_mic_submenu(Microphone* mic, int indent_level)
{
    param_t params[] =
    {
      {
        .set = (funct)mic_set_rhythm_generator_index,
        .get = (funct)mic_get_rhythm_generator_index,
        .enter = (funct)enter_rhythm_submenu,
        .self = mic,
        .type = 'o',
        .init = 0,
        .increment = 1,
        .name = "[ENTER] mic_set_rhythm_generator_index",
       },
      {
        .set = (funct)mic_set_count_out_n,
        .get = (funct)mic_get_count_out_n,
        .enter = NULL,
        .self = mic,
        .type = 'i',
        .init = 8,
        .increment = 1,
        .name = "mic_set_count_out_n",
      },
      {
        .set = (funct)mic_set_should_play_beat_bell,
        .get = (funct)mic_get_should_play_beat_bell,
        .enter = NULL,
        .self = mic,
        .type = 'i',
        .init = 1,
        .increment = 1,
        .name = "mic_set_should_play_beat_bell",
      },
      {
        .set = (funct)mic_set_quantization_order,
        .get = (funct)mic_get_quantization_order,
        .enter = NULL,
        .self = mic,
        .type = 'i',
        .init = 8,
        .increment = 1,
        .name = "mic_set_quantization_order",
      },
    };
  int num_params = sizeof(params) / sizeof(params[0]);
  return cycle_through_paramaters_and_take_get_input("mic", params, num_params, "RHYTHM CONTROLLER SUBMENU", indent_level);
}

/*--------------------------------------------------------------------*/
double robot_strength = 1.0;
int    robot_solenoid = 0;
int    eye_depth      = 2;
int    robot_sustain_mode = 0;
double eye_speed      = 100;
void   robot_set_solenoid     (void* ignored, int    solenoid){clip(robot_solenoid, solenoid, 0, 7)};
void   robot_set_strength     (void* ignored, double strength){clip(robot_strength, strength, 0, 1)};
void   robot_set_eye_depth    (void* ignored, int    depth   ){clip(eye_depth, depth, 1, 5)};
void   robot_set_eye_speed    (void* ignored, double speed   ){clip(eye_speed, speed, 33, 2000)};
int    robot_get_solenoid     (void* ignored){return robot_solenoid;}
double robot_get_strength     (void* ignored){return robot_strength;}
int    robot_get_eye_depth    (void* ignored){return eye_depth;}
double robot_get_eye_speed    (void* ignored){return eye_speed;}

int    robot_get_sustain_mode (void* ignored){return robot_sustain_mode;}


int    send_robot_tap         (Robot* robot, int ignored){robot_send_message(robot, robot_cmd_tap, robot_strength); return 1;}
int    send_robot_tap_specific(Robot* robot, int ignored){robot_send_message(robot, robot_cmd_tap_specific, robot_solenoid, robot_strength); return 1;}
int    send_robot_bell        (Robot* robot, int ignored){robot_send_message(robot, robot_cmd_bell, robot_strength); return 1;}

int    send_robot_sustain_mode(Robot* robot, int mode){robot_send_message(robot, robot_cmd_set_sustain_mode, robot_sustain_mode=mode%2); return 1;}
int    send_robot_note_on     (Robot* robot, int ignored){robot_send_message(robot, robot_cmd_note_on, robot_solenoid); return 1;}
int    send_robot_note_off    (Robot* robot, int ignored){robot_send_message(robot, robot_cmd_note_off, robot_solenoid); return 1;}
int    send_robot_play        (Robot* robot, int ignored){robot_send_message(robot, robot_cmd_play_note_for_duration, robot_solenoid, 1.0); return 1;}
int    send_robot_all_off     (Robot* robot, int ignored){robot_send_message(robot, robot_cmd_all_notes_off); return 1;}


int    send_robot_blink       (Robot* robot, int ignored){robot_send_message(robot, robot_cmd_eye_blink); return 1;}
int    send_robot_roll        (Robot* robot, int ignored){robot_send_message(robot, robot_cmd_eye_roll, eye_depth); return 1;}
int    send_robot_no          (Robot* robot, int ignored){robot_send_message(robot, robot_cmd_eye_no, eye_depth, eye_speed); return 1;}
int    send_robot_yes         (Robot* robot, int ignored){robot_send_message(robot, robot_cmd_eye_yes, eye_depth, eye_speed); return 1;}
int    send_robot_inquisitive (Robot* robot, int ignored){robot_send_message(robot, robot_cmd_eye_inquisitive); return 1;}
int    send_robot_focused     (Robot* robot, int ignored){robot_send_message(robot, robot_cmd_eye_focused); return 1;}
int    send_robot_surprised   (Robot* robot, int ignored){robot_send_message(robot, robot_cmd_eye_surprised); return 1;}
int    send_robot_neutral_size(Robot* robot, int ignored){robot_send_message(robot, robot_cmd_eye_neutral_size); return 1;}
int    send_robot_neutral_pos (Robot* robot, int ignored){robot_send_message(robot, robot_cmd_eye_neutral_position); return 1;}

int enter_robot_communication_submenu(Robot* robot, int indent_level)
{
    param_t params[] =
    {
      {
        .set = (funct)robot_set_solenoid,
        .get = (funct)robot_get_solenoid,
        .enter = (funct)send_robot_tap_specific,
        .self = robot,
        .type = 'i',
        .init = 0,
        .increment = 1,
        .name = "robot_set_solenoid",
      },
      {
        .set = (funct)robot_set_strength,
        .get = (funct)robot_get_strength,
        .enter = (funct)send_robot_tap_specific,
        .self = robot,
        .type = 'd',
        .init = 1,
        .increment = 0.1,
        .name = "robot_set_strength",
      },
      {
        .set = NULL,
        .get = NULL,
        .enter = (funct)send_robot_tap,
        .self = robot,
        .type = 'e',
        .init = 0,
        .increment = 0,
        .name = "robot_send_message(robot_cmd_tap, strength)",
      },
      {
        .set = NULL,
        .get = NULL,
        .enter = (funct)send_robot_tap_specific,
        .self = robot,
        .type = 'e',
        .init = 0,
        .increment = 0,
        .name = "robot_send_message(robot_cmd_tap_specific, solenoid, strength)",
      },
      {
        .set = NULL,
        .get = NULL,
        .enter = (funct)send_robot_bell,
        .self = robot,
        .type = 'e',
        .init = 0,
        .increment = 0,
        .name = "robot_send_message(robot_cmd_bell, strength)",
      },
      {
        .set = (funct)send_robot_sustain_mode,
        .get = (funct)robot_get_sustain_mode,
        .enter = NULL,
        .self = robot,
        .type = 'i',
        .init = 0,
        .increment = 1,
        .name = "robot_send_message(robot_cmd_set_sustain_mode, mode)",
      },
      {
        .set = NULL,
        .get = NULL,
        .enter = (funct)send_robot_note_on,
        .self = robot,
        .type = 'e',
        .init = 0,
        .increment = 0,
        .name = "robot_send_message(robot_cmd_note_on, solenoid)",
      },
      {
        .set = NULL,
        .get = NULL,
        .enter = (funct)send_robot_note_off,
        .self = robot,
        .type = 'e',
        .init = 0,
        .increment = 0,
        .name = "robot_send_message(robot_cmd_note_off, solenoid)",
      },
      {
        .set = NULL,
        .get = NULL,
        .enter = (funct)send_robot_play,
        .self = robot,
        .type = 'e',
        .init = 0,
        .increment = 0,
        .name = "robot_send_message(robot_cmd_play_note_for_duration, solenoid, 1.0)",
      },
      {
        .set = NULL,
        .get = NULL,
        .enter = (funct)send_robot_all_off,
        .self = robot,
        .type = 'e',
        .init = 0,
        .increment = 0,
        .name = "robot_send_message(robot_cmd_all_notes_off)",
      },
      {
        .set = NULL,
        .get = NULL,
        .enter = (funct)send_robot_blink,
        .self = robot,
        .type = 'e',
        .init = 0,
        .increment = 0,
        .name = "robot_send_message(robot_cmd_eye_blink)",
      },
      {
        .set = (funct)robot_set_eye_depth,
        .get = (funct)robot_get_eye_depth,
        .enter = (funct)send_robot_yes,
        .self = robot,
        .type = 'i',
        .init = 0,
        .increment = 1,
        .name = "robot_set_eye_depth",
      },
      {
        .set = (funct)robot_set_eye_speed,
        .get = (funct)robot_get_eye_speed,
        .enter = (funct)send_robot_yes,
        .self = robot,
        .type = 'd',
        .init = 1,
        .increment = 5,
        .name = "robot_set_eye_speed",
      },
      {
        .set = NULL,
        .get = NULL,
        .enter = (funct)send_robot_roll,
        .self = robot,
        .type = 'e',
        .init = 0,
        .increment = 0,
        .name = "robot_send_message(robot_cmd_eye_roll, depth)",
      },
      {
        .set = NULL,
        .get = NULL,
        .enter = (funct)send_robot_yes,
        .self = robot,
        .type = 'e',
        .init = 0,
        .increment = 0,
        .name = "robot_send_message(robot_cmd_eye_yes, depth, speed)",
      },
      {
        .set = NULL,
        .get = NULL,
        .enter = (funct)send_robot_no,
        .self = robot,
        .type = 'e',
        .init = 0,
        .increment = 0,
        .name = "robot_send_message(robot_cmd_eye_no, depth, speed)",
      },
      {
        .set = NULL,
        .get = NULL,
        .enter = (funct)send_robot_inquisitive,
        .self = robot,
        .type = 'e',
        .init = 0,
        .increment = 0,
        .name = "robot_send_message(robot_cmd_eye_inquisitive)",
      },
      {
        .set = NULL,
        .get = NULL,
        .enter = (funct)send_robot_focused,
        .self = robot,
        .type = 'e',
        .init = 0,
        .increment = 0,
        .name = "robot_send_message(robot_cmd_eye_focused)",
      },
      {
        .set = NULL,
        .get = NULL,
        .enter = (funct)send_robot_surprised,
        .self = robot,
        .type = 'e',
        .init = 0,
        .increment = 0,
        .name = "robot_send_message(robot_cmd_eye_surprised)",
      },
      {
        .set = NULL,
        .get = NULL,
        .enter = (funct)send_robot_neutral_size,
        .self = robot,
        .type = 'e',
        .init = 0,
        .increment = 0,
        .name = "robot_send_message(robot_cmd_eye_neutral_size)",
      },
      {
        .set = NULL,
        .get = NULL,
        .enter = (funct)send_robot_neutral_pos,
        .self = robot,
        .type = 'e',
        .init = 0,
        .increment = 0,
        .name = "robot_send_message(robot_cmd_eye_neutral_pos)",
       },
    };
  int num_params = sizeof(params) / sizeof(params[0]);
  return cycle_through_paramaters_and_take_get_input("robot", params, num_params, "TEENSY COMMUNICATION SUBMENU", indent_level);
}

/*--------------------------------------------------------------------*/
int enter_beat_tracking_submenu(BTT* btt, int indent_level)
{
  param_t params[] =
    {
      {
        .set = (funct)btt_set_tracking_mode,
        .get = (funct)btt_get_tracking_mode,
        .enter = NULL,
        .self = btt,
        .type = 'i',
        .init = BTT_DEFAULT_TRACKING_MODE,
        .increment = 1,
        .name = "btt_set_tracking_mode",
      },
      {
        .set = (funct)btt_set_metronome_bpm,
        .get = (funct)btt_get_tempo_bpm,
        .enter = NULL,
        .self = btt,
        .type = 'd',
        .init = BTT_DEFAULT_LOG_GAUSSIAN_TEMPO_WEIGHT_MEAN,
        .increment = 4,
        .name = "btt_set_metronome_bpm",
      },
      {
        .set = (funct)btt_set_noise_cancellation_threshold,
        .get = (funct)btt_get_noise_cancellation_threshold,
        .enter = NULL,
        .self = btt,
        .type = 'd',
        .init = BTT_DEFAULT_NOISE_CANCELLATION_THRESHOLD,
        .increment = 1,
        .name = "btt_set_noise_cancellation_threshold",
      },
      {
        .set = (funct)btt_set_use_amplitude_normalization,
        .get = (funct)btt_get_use_amplitude_normalization,
        .enter = NULL,
        .self = btt,
        .type = 'i',
        .init = BTT_DEFAULT_USE_AMP_NORMALIZATION,
        .increment = 1,
        .name = "btt_set_use_amplitude_normalization",
      },
      {
        .set = (funct)btt_set_spectral_compression_gamma,
        .get = (funct)btt_get_spectral_compression_gamma,
        .enter = NULL,
        .self = btt,
        .type = 'd',
        .init = BTT_DEFAULT_SPECTRAL_COMPRESSION_GAMMA,
        .increment = 0.05,
        .name = "btt_set_spectral_compression_gamma",
      },
      {
        .set = (funct)btt_set_oss_filter_cutoff,
        .get = (funct)btt_get_oss_filter_cutoff,
        .enter = NULL,
        .self = btt,
        .type = 'd',
        .init = BTT_DEFAULT_OSS_FILTER_CUTOFF,
        .increment = 0.5,
        .name = "btt_set_oss_filter_cutoff",
      },
      {
        .set = (funct)btt_set_onset_threshold,
        .get = (funct)btt_get_onset_threshold,
        .enter = NULL,
        .self = btt,
        .type = 'd',
        .init = BTT_DEFAULT_ONSET_TREHSHOLD,
        .increment = 0.1,
        .name = "btt_set_onset_threshold",
      },
      {
        .set = (funct)btt_set_onset_threshold_min,
        .get = (funct)btt_get_onset_threshold_min,
        .enter = NULL,
        .self = btt,
        .type = 'd',
        .init = BTT_DEFAULT_ONSET_TREHSHOLD_MIN,
        .increment = 0.01,
        .name = "btt_set_onset_threshold_min",
      },
      {
        .set = (funct)btt_set_autocorrelation_exponent,
        .get = (funct)btt_get_autocorrelation_exponent,
        .enter = NULL,
        .self = btt,
        .type = 'd',
        .init = BTT_DEFAULT_AUTOCORRELATION_EXPONENT,
        .increment = 0.1,
        .name = "btt_set_autocorrelation_exponent",
      },
      {
        .set = (funct)btt_set_min_tempo,
        .get = (funct)btt_get_min_tempo,
        .enter = NULL,
        .self = btt,
        .type = 'd',
        .init = BTT_DEFAULT_MIN_TEMPO,
        .increment = 4,
        .name = "btt_set_min_tempo",
      },
      {
        .set = (funct)btt_set_max_tempo,
        .get = (funct)btt_get_max_tempo,
        .enter = NULL,
        .self = btt,
        .type = 'd',
        .init = BTT_DEFAULT_MAX_TEMPO,
        .increment = 4,
        .name = "btt_set_max_tempo",
      },
      {
        .set = (funct)btt_set_num_tempo_candidates,
        .get = (funct)btt_get_num_tempo_candidates,
        .enter = NULL,
        .self = btt,
        .type = 'i',
        .init = BTT_DEFAULT_NUM_TEMPO_CANDIDATES,
        .increment = 1,
        .name = "btt_set_num_tempo_candidates",
      },
      {
        .set = (funct)btt_set_gaussian_tempo_histogram_decay,
        .get = (funct)btt_get_gaussian_tempo_histogram_decay,
        .enter = NULL,
        .self = btt,
        .type = 'd',
        .init = BTT_DEFAULT_GAUSSIAN_TEMPO_HISTOGRAM_DECAY,
        .increment = 0.00001,
        .name = "btt_set_gaussian_tempo_histogram_decay",
      },
      {
        .set = (funct)btt_set_gaussian_tempo_histogram_width,
        .get = (funct)btt_get_gaussian_tempo_histogram_width,
        .enter = NULL,
        .self = btt,
        .type = 'd',
        .init = BTT_DEFAULT_GAUSSIAN_TEMPO_HISTOGRAM_WIDTH,
        .increment = 0.5,
        .name = "btt_set_gaussian_tempo_histogram_width",
      },
      {
        .set = (funct)btt_set_log_gaussian_tempo_weight_mean,
        .get = (funct)btt_get_log_gaussian_tempo_weight_mean,
        .enter = NULL,
        .self = btt,
        .type = 'd',
        .init = BTT_DEFAULT_LOG_GAUSSIAN_TEMPO_WEIGHT_MEAN,
        .increment = 4,
        .name = "btt_set_log_gaussian_tempo_weight_mean",
      },
      {
        .set = (funct)btt_set_log_gaussian_tempo_weight_width,
        .get = (funct)btt_get_log_gaussian_tempo_weight_width,
        .enter = NULL,
        .self = btt,
        .type = 'd',
        .init = BTT_DEFAULT_LOG_GAUSSIAN_TEMPO_WEIGHT_WIDTH,
        .increment = 4,
        .name = "btt_set_log_gaussian_tempo_weight_width",
      },
      {
        .set = (funct)btt_set_cbss_alpha,
        .get = (funct)btt_get_cbss_alpha,
        .enter = NULL,
        .self = btt,
        .type = 'd',
        .init = BTT_DEFAULT_CBSS_ALPHA,
        .increment = 0.01,
        .name = "btt_set_cbss_alpha",
      },
      {
        .set = (funct)btt_set_cbss_eta,
        .get = (funct)btt_get_cbss_eta,
        .enter = NULL,
        .self = btt,
        .type = 'd',
        .init = BTT_DEFAULT_CBSS_ETA,
        .increment = 5,
        .name = "btt_set_cbss_eta",
      },
      {
        .set = (funct)btt_set_beat_prediction_adjustment,
        .get = (funct)btt_get_beat_prediction_adjustment,
        .enter = NULL,
        .self = btt,
        .type = 'i',
        .init = BTT_DEFAULT_BEAT_PREDICTION_ADJUSTMENT,
        .increment = 1,
        .name = "btt_set_beat_prediction_adjustment",
      },
      {
        .set = (funct)btt_set_predicted_beat_trigger_index,
        .get = (funct)btt_get_predicted_beat_trigger_index,
        .enter = NULL,
        .self = btt,
        .type = 'i',
        .init = BTT_DEFAULT_PREDICTED_BEAT_TRIGGER_INDEX,
        .increment = 1,
        .name = "btt_set_predicted_beat_trigger_index",
      },
      {
        .set = (funct)btt_set_predicted_beat_gaussian_width,
        .get = (funct)btt_get_predicted_beat_gaussian_width,
        .enter = NULL,
        .self = btt,
        .type = 'd',
        .init = BTT_DEFAULT_PREDICTED_BEAT_GAUSSIAN_WIDTH,
        .increment = 1,
        .name = "btt_set_predicted_beat_gaussian_width",
      },
      {
        .set = (funct)btt_set_ignore_spurious_beats_duration,
        .get = (funct)btt_get_ignore_spurious_beats_duration,
        .enter = NULL,
        .self = btt,
        .type = 'd',
        .init = BTT_DEFAULT_IGNORE_SPURIOUS_BEATS_DURATION,
        .increment = 5,
        .name = "btt_set_ignore_spurious_beats_duration",
      },
      {
        .set = (funct)btt_set_count_in_n,
        .get = (funct)btt_get_count_in_n,
        .enter = NULL,
        .self = btt,
        .type = 'i',
        .init = BTT_DEFAULT_COUNT_IN_N,
        .increment = 1,
        .name = "btt_set_count_in_n",
      },
    };
  int num_params = sizeof(params) / sizeof(params[0]);
  return cycle_through_paramaters_and_take_get_input("btt", params, num_params, "BEAT TRACKING SUBMENU", indent_level);
}

/*--------------------------------------------------------------------*/
int enter_rhythm_submenu(Microphone* mic, int indent_level)
{
  Rhythm* rhythm = mic_get_rhythm_generator(mic);
  const char* name = rhythm_get_name(rhythm);
  
  
  if(strcmp(name, "Histogram") == 0)
    {
      param_t params[] =
        {
          {
            .set = (funct)rhythm_histogram_set_is_inverse,
            .get = (funct)rhythm_histogram_get_is_inverse,
            .enter = NULL,
            .self = rhythm,
            .type = 'i',
            .init = 0,
            .increment = 1,
            .name = "rhythm_histogram_set_is_inverse",
          },
          {
            .set = (funct)rhythm_histogram_set_num_beats,
            .get = (funct)rhythm_histogram_get_num_beats,
            .enter = NULL,
            .self = rhythm,
            .type = 'i',
            .init = 4,
            .increment = 1,
            .name = "rhythm_histogram_set_num_beats",
          },
          {
            .set = (funct)rhythm_histogram_set_subdivisions_per_beat,
            .get = (funct)rhythm_histogram_get_subdivisions_per_beat,
            .enter = NULL,
            .self = rhythm,
            .type = 'i',
            .init = 4,
            .increment = 1,
            .name = "rhythm_histogram_set_subdivisions_per_beat",
          },
          {
            .set = (funct)rhythm_histogram_set_nonlinear_exponent,
            .get = (funct)rhythm_histogram_get_nonlinear_exponent,
            .enter = NULL,
            .self = rhythm,
            .type = 'd',
            .init = 1,
            .increment = 0.01,
            .name = "rhythm_histogram_set_nonlinear_exponent",
          },
          {
            .set = (funct)rhythm_histogram_set_decay_coefficient,
            .get = (funct)rhythm_histogram_get_decay_coefficient,
            .enter = NULL,
            .self = rhythm,
            .type = 'd',
            .init = 0.75,
            .increment = 0.01,
            .name = "rhythm_histogram_set_decay_coefficient",
          },
        };
      int num_params = sizeof(params) / sizeof(params[0]);
      return cycle_through_paramaters_and_take_get_input("rhythm", params, num_params, "HISTOGRAM RHYTHM SUBMENU", indent_level);
    }
  else if(strcmp(name, "Two_Beat_Delay") == 0)
    {
      param_t params[] =
        {
          {
            .set = (funct)rhythm_two_beat_delay_set_beats_delay,
            .get = (funct)rhythm_two_beat_delay_get_beats_delay,
            .enter = NULL,
            .self = rhythm,
            .type = 'd',
            .init = 2,
            .increment = 1,
            .name = "rhythm_two_beat_delay_set_beats_delay",
          },
        };
      int num_params = sizeof(params) / sizeof(params[0]);
      return cycle_through_paramaters_and_take_get_input("rhythm", params, num_params, "TWO BEAT DELAY RHYTHM SUBMENU", indent_level);
    }
  else if(strcmp(name, "OSC") == 0)
    {
      param_t params[] =
        {
          {
            .set = (funct)rhythm_osc_set_robot_osc_id,
            .get = (funct)rhythm_osc_get_robot_osc_id,
            .enter = NULL,
            .self = rhythm,
            .type = 'i',
            .init = 0,
            .increment = 1,
            .name = "rhythm_osc_set_robot_osc_id",
          },
        };
      int num_params = sizeof(params) / sizeof(params[0]);
      return cycle_through_paramaters_and_take_get_input("rhythm", params, num_params, "OSC RHYTHM SUBMENU", indent_level);
    }
  else if(strcmp(name, "Quantized_Delay") == 0)
    {
      param_t params[] =
        {
          {
            .set = (funct)rhythm_quantized_delay_set_beats_delay,
            .get = (funct)rhythm_quantized_delay_get_beats_delay,
            .enter = NULL,
            .self = rhythm,
            .type = 'd',
            .init = 2,
            .increment = 1,
            .name = "rhythm_quantized_delay_set_beats_delay",
          },
          {
            .set = (funct)rhythm_quantized_delay_set_quantizer_update_interval,
            .get = (funct)rhythm_quantized_delay_get_quantizer_update_interval,
            .enter = NULL,
            .self = rhythm,
            .type = 'i',
            .init = 100000,
            .increment = 1000,
            .name = "rhythm_quantized_delay_set_quantizer_update_interval",
          },
        };
      int num_params = sizeof(params) / sizeof(params[0]);
      return cycle_through_paramaters_and_take_get_input("rhythm", params, num_params, "TWO BEAT DELAY RHYTHM SUBMENU", indent_level);
    }
  else
    {
      return 1;
    }
}
/*--------------------------------------------------------------------*/
int main(void)
{
  fprintf(stderr, " ____         ____              _             _\r\n");
  fprintf(stderr, "|  _ \\ _ __  / ___|  __ _ _   _(_) __ _  __ _| | ___  ___\r\n");
  fprintf(stderr, "| | | | '__| \\___ \\ / _` | | | | |/ _` |/ _` | |/ _ \\/ __|\r\n");
  fprintf(stderr, "| |_| | | _   ___) | (_| | |_| | | (_| | (_| | |  __/\\__ \\\r\n");
  fprintf(stderr, "|____/|_|(_) |____/ \\__, |\\__,_|_|\\__, |\\__, |_|\\___||___/\r\n");
  fprintf(stderr, "                       |_|        |___/ |___/\r\n");

  //fprintf(stderr, "o-o              o-o                        o\r\n");
  //fprintf(stderr, "|  \\            |               o           |\r\n");
  //fprintf(stderr, "|   O o-o        o-o   o-o o  o   o--o o--o | o-o o-o\r\n");
  //fprintf(stderr, "|  /  |             | |  | |  | | |  | |  | | |-'  \\\r\n");
  //fprintf(stderr, "o-o   o   O     o--o   o-O o--o | o--O o--O o o-o o-o\r\n");
  //fprintf(stderr, "                         |           |    |\r\n");
  //fprintf(stderr, "                         o        o--o o--o\r\n");
  
  Microphone*  mic = mic_new();
  if(mic == NULL) {perror("Unable to create microphone object"); exit(-1);}
  /* mic_new might print Arduino firmware version */
  
  if(!main_init_osc_communication(mic))
    {perror("Unable to init OSC communication\r\n"); exit(-1);}
 
  fprintf(stderr, "Raspi is running software version %s\r\n\r\n", __SQ_VERSION__);
  fprintf(stderr, " -- 'Q' to quit\r\n");
  fprintf(stderr, " -- [ENTER] to enter submenu\r\n");
  fprintf(stderr, " -- 'q' to exit submenu\r\n");
  fprintf(stderr, " -- → and ← to scroll through menu\r\n");
  fprintf(stderr, " -- ↑ and ↓ to change parameter values\r\n");
  fprintf(stderr, " -- 'd' to set the default paramater\r\n");
  fprintf(stderr, " -- SHORTCUTS \r\n");
  fprintf(stderr, "\t -- 'c' for count-in mode \r\n");
  fprintf(stderr, "\t -- 'l' for tempo-locked beat-tracking mode \r\n");
  fprintf(stderr, "\t -- 'L' for unlocked (regular) beat-tracking mode \r\n");
  fprintf(stderr, "\r\n");

  //mic_set_rhythm_generator       (mic, rhythm_random_beat_from_list_new);
  //mic_set_rhythm_generator       (mic, rhythm_quantized_delay_new);
  mic_set_rhythm_generator       (mic, rhythm_histogram_new);
  //mic_set_count_out_n            (mic, 0);
  //mic_set_should_play_beat_bell  (mic, 0);

  
  global_beat_tracker  = mic_get_btt(mic);
  Robot* robot = mic_get_robot(mic);
#if defined __linux__
  btt_set_beat_prediction_adjustment(global_beat_tracker, 6.000000);
#elif defined __APPLE__
  btt_set_beat_prediction_adjustment(global_beat_tracker, 18.000000);
#endif
  btt_set_tracking_mode(global_beat_tracker, BTT_COUNT_IN_TRACKING);
  
  //btt_set_min_tempo(global_beat_tracker, 80);
  //btt_set_max_tempo(global_beat_tracker, 160);
  
  auPlay((Audio*)mic);
  

  param_t params[] =
    {
      {
        .set = NULL,
        .get = NULL,
        .enter = (funct)enter_mic_submenu,
        .self = mic,
        .type = 'e',
        .init = 0,
        .increment = 0,
        .name = "Rhythm Controller Submenu",
      },
      {
        .set = NULL,
        .get = NULL,
        .enter = (funct)enter_rhythm_submenu,
        .self = mic,
        .type = 'e',
        .init = 0,
        .increment = 0,
        .name = "Rhythm Submenu",
      },
      {
        .set = NULL,
        .get = NULL,
        .enter = (funct)enter_beat_tracking_submenu,
        .self = global_beat_tracker,
        .type = 'e',
        .init = 0,
        .increment = 0,
        .name = "Beat Tracking Submenu",
      },
      {
        .set = NULL,
        .get = NULL,
        .enter = (funct)enter_robot_communication_submenu,
        .self = robot,
        .type = 'e',
        .init = 0,
        .increment = 0,
        .name = "Teensy Communication Submenu",
      },
    };
  
  i_hate_canonical_input_processing();
  cycle_through_paramaters_and_take_get_input("null", params, sizeof(params) / sizeof(params[0]), "MAIN MENU", 0);
  make_stdin_cannonical_again();
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
