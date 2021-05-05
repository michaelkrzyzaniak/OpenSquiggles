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
//gcc op2.c core/*.c ../Robot_Communication_Framework/*.c ../Beat-and-Tempo-Tracking/src/*.c Rhythm_Generators/*.c extras/*.c -framework CoreMidi -framework Carbon -framework AudioToolbox -O2 -o op2

//Linux compile with:
//sudo apt-get install libasound2-dev
//gcc op2.c core/*.c ../Robot_Communication_Framework/*.c ../Beat-and-Tempo-Tracking/src/*.c Rhythm_Generators/*.c extras/*.c lib/dywapitchtrack/src/*.c -lasound -lm -lpthread -lrt -O2 -o op2

//gcc op2.c core/*.c ../Robot_Communication_Framework/*.c ../Beat-and-Tempo-Tracking/src/*.c Rhythm_Generators/*.c extras/*.c -lasound -lm -lpthread -lrt -O2 -o op2

#include "core/Matrix.h"
#include "core/Timestamp.h"
//#include "core/Harmonizer.h"
#include "core/HarmonizerController.h"


#include <stdio.h>

void  make_stdin_cannonical_again();
void i_hate_canonical_input_processing(void);

HarmonizerController* global_controller;

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
int cycle_through_paramaters_and_get_input(const char* object_name, param_t* params, int num_params, const char* submenu_message, int indent_level)
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
            harmonizer_controller_clear(global_controller);
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
int enter_main_menu(HarmonizerController* controller, int indent_level, Poly_Harmonizer* harmonizer, Organ_Pipe_Filter* filter)
{
    param_t params[] =
    {
      {
        .set = (funct)organ_pipe_filter_set_reduction_coefficient,
        .get = (funct)organ_pipe_filter_get_reduction_coefficient,
        .enter = NULL,
        .self = filter,
        .type = 'd',
        .init = ORGAN_PIPE_FILTER_DEFAULT_REDUCTION_COEFFICIENT,
        .increment = 0.05,
        .name = "organ_pipe_filter_set_reduction_coefficient",
       },
      {
        .set = (funct)organ_pipe_filter_set_gate_thresh,
        .get = (funct)organ_pipe_filter_get_gate_thresh,
        .enter = NULL,
        .self = filter,
        .type = 'd',
        .init = ORGAN_PIPE_FILTER_DEFAULT_GATE_THRESH,
        .increment = 5,
        .name = "organ_pipe_filter_set_gate_thresh",
      },
      {
        .set = (funct)organ_pipe_filter_set_noise_cancel_thresh,
        .get = (funct)organ_pipe_filter_get_noise_cancel_thresh,
        .enter = NULL,
        .self = filter,
        .type = 'd',
        .init = ORGAN_PIPE_FILTER_DEFAULT_NOISE_CANCEL_THRESH,
        .increment = 0.05,
        .name = "organ_pipe_filter_set_noise_cancel_thresh",
      },
      {
        .set = (funct)poly_harmonizer_set_on_for,
        .get = (funct)poly_harmonizer_get_on_for,
        .enter = NULL,
        .self = harmonizer,
        .type = 'i',
        .init = POLY_HARMONIZER_DEFAULT_ON_FOR,
        .increment = 1,
        .name = "poly_harmonizer_set_on_for",
       },
      {
        .set = (funct)poly_harmonizer_set_off_for,
        .get = (funct)poly_harmonizer_get_off_for,
        .enter = NULL,
        .self = harmonizer,
        .type = 'i',
        .init = POLY_HARMONIZER_DEFAULT_OFF_FOR,
        .increment = 1,
        .name = "poly_harmonizer_set_off_for",
      },
      {
        .set = (funct)poly_harmonizer_set_min_note,
        .get = (funct)poly_harmonizer_get_min_note,
        .enter = NULL,
        .self = harmonizer,
        .type = 'd',
        .init = POLY_HARMONIZER_DEFAULT_MIN_NOTE,
        .increment = 1,
        .name = "poly_harmonizer_set_min_note",
      },
      {
        .set = (funct)poly_harmonizer_set_max_note,
        .get = (funct)poly_harmonizer_get_max_note,
        .enter = NULL,
        .self = harmonizer,
        .type = 'd',
        .init = POLY_HARMONIZER_DEFAULT_MAX_NOTE,
        .increment = 1,
        .name = "poly_harmonizer_set_max_note",
       },
      {
        .set = (funct)poly_harmonizer_set_resolution,
        .get = (funct)poly_harmonizer_get_resolution,
        .enter = NULL,
        .self = harmonizer,
        .type = 'd',
        .init = POLY_HARMONIZER_DEFAULT_RESOLUTION,
        .increment = 1,
        .name = "poly_harmonizer_set_resolution",
      },
      {
        .set = (funct)poly_harmonizer_set_max_polyphony,
        .get = (funct)poly_harmonizer_get_max_polyphony,
        .enter = NULL,
        .self = harmonizer,
        .type = 'i',
        .init = POLY_HARMONIZER_DEFAULT_MAX_POLYPHONY,
        .increment = 1,
        .name = "poly_harmonizer_set_max_polyphony",
      },
      {
        .set = (funct)poly_harmonizer_set_delta,
        .get = (funct)poly_harmonizer_get_delta,
        .enter = NULL,
        .self = harmonizer,
        .type = 'd',
        .init = POLY_HARMONIZER_DEFAULT_DELTA,
        .increment = 0.1,
        .name = "poly_harmonizer_set_delta",
      },
      {
        .set = (funct)poly_harmonizer_set_M,
        .get = (funct)poly_harmonizer_get_M,
        .enter = NULL,
        .self = harmonizer,
        .type = 'i',
        .init = POLY_HARMONIZER_DEFAULT_M,
        .increment = 1,
        .name = "poly_harmonizer_set_M",
      }
    };
  int num_params = sizeof(params) / sizeof(params[0]);
  return cycle_through_paramaters_and_get_input("self", params, num_params, "MAIN MENU", indent_level);
}

int main(int argc, char* argv[])
{
  global_controller = harmonizer_controller_new();
  if(global_controller == NULL)
    {perror("unable to create Harmonizer Controller"); return -1;}
  
  if(argc > 1)
    {
      while(--argc > 0)
        {
          ++argv;
          MKAiff* aiff = aiffWithContentsOfFile(*argv);
          if(aiff != NULL)
            {
              double aiff_secs = aiffDurationInSeconds(aiff);
              fprintf(stderr, "processing %s -- %.1f secs -- %i channels ...\r\n", *argv, aiff_secs, aiffNumChannels(aiff));
              timestamp_microsecs_t start = timestamp_get_current_time();
              auProcessOffline((Audio*)global_controller, aiff);
              timestamp_microsecs_t end = timestamp_get_current_time();
              double process_secs = (end-start)/1000000.0;
              fprintf(stderr, "Done in %.2f seconds -- %.2f%% realtime\r\n", process_secs, 100*process_secs/aiff_secs);
              aiffDestroy(aiff);
            }
          else
            fprintf(stderr, "%s is not a valid AIFF or wav file\r\n", *argv);
        }
    }
  else
    {
      i_hate_canonical_input_processing();
      auPlay((Audio*)global_controller);
      
      Poly_Harmonizer* harmonizer = harmonizer_controller_get_harmonizer(global_controller);
      Organ_Pipe_Filter* filter = poly_harmonizer_get_organ_pipe_filter(harmonizer);
      
      enter_main_menu(global_controller, 0, harmonizer, filter);

      make_stdin_cannonical_again();
    }
  
  global_controller = (HarmonizerController*)auSubclassDestroy((Audio*)global_controller);
  return 0;
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
