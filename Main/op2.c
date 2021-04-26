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
//gcc op2.c core/*.c ../Robot_Communication_Framework/*.c ../Beat-and-Tempo-Tracking/src/*.c Rhythm_Generators/*.c extras/*.c lib/dywapitchtrack/src/*.c -framework CoreMidi -framework Carbon -framework AudioToolbox -O2 -o op2

//Linux compile with:
//sudo apt-get install libasound2-dev
//gcc op2.c core/*.c ../Robot_Communication_Framework/*.c ../Beat-and-Tempo-Tracking/src/*.c Rhythm_Generators/*.c extras/*.c lib/dywapitchtrack/src/*.c -lasound -lm -lpthread -lrt -O2 -o op2

#include "core/Matrix.h"
#include "core/Timestamp.h"
//#include "core/Harmonizer.h"
#include "core/HarmonizerController.h"


#include <stdio.h>

void  make_stdin_cannonical_again();
void i_hate_canonical_input_processing(void);

/*
#include <math.h> //testing
#include <stdlib.h> //testing
#include <string.h> //testing
#include "DFT.h"

int main(void)
{
  int window_size = 2048;
  int fft_N = window_size * 2;
  
  float* window = calloc(window_size, sizeof(*window));
  float* real   = calloc(fft_N, sizeof(*real));
  float* imag   = calloc(fft_N, sizeof(*imag));
  
  float sample_rate = 44100;
  float cps;
  float freq;
  float phase;
  
  int i, trial;
  
  float ratio = 0;
  
  for(trial = 0; trial <= 100; trial++)
    {
      phase = 0;
      cps = dft_frequency_of_bin(250, sample_rate, fft_N);
      freq = cps * 2 * M_PI / sample_rate;
  
      memset(real, 0, fft_N * sizeof(*real));
      memset(imag, 0, fft_N * sizeof(*imag));
  
      for(i=0; i<floor(window_size*ratio); i++)
        {
          real[i] = cos(phase);
          phase += freq;
        }

      cps = dft_frequency_of_bin(666, sample_rate, fft_N);
      freq = cps * 2 * M_PI / sample_rate;
  
      for(;i<window_size; i++)
        {
          real[i] = cos(phase);
          phase += freq;
        }

      dft_init_hamming_window(window, window_size);
      dft_apply_window(real, window, window_size);
      dft_real_forward_dft(real, imag, fft_N);
  
      dft_rect_to_polar(real, imag, fft_N);
  
      fprintf(stderr, "%i\t%f\t%f\t%f\r\n", trial, real[250], real[666], real[250] + real[666]);
  
      ratio += 0.01;
      //for(i=0; i<window_size; i++)
        //fprintf(stderr, "%f\r\n", real[i]);
    }
}
*/

int main(int argc, char* argv[])
{
  HarmonizerController* controller = harmonizer_controller_new();
  if(controller == NULL)
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
              auProcessOffline((Audio*)controller, aiff);
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
      auPlay((Audio*)controller);
      for(;;)
        {
          char c = getchar();
          switch(c)
            {
              case 'q':
                goto out;
                break;
              case 'c':
                harmonizer_controller_clear(controller);
                break;
              default: break;
            
            }
        }
      out:
      make_stdin_cannonical_again();
    }
  
  controller = (HarmonizerController*)auSubclassDestroy((Audio*)controller);
  return 0;
}

/*
int _main(void)
{
  unsigned num_tests = 100;
  unsigned a_rows = 2411;
  unsigned a_cols = 1235;
  unsigned b_cols = 1;

  Matrix* a = matrix_new(a_rows, a_cols);
  Matrix* b = matrix_new(a_cols, b_cols);
  Matrix* result_1 = matrix_new(a_rows, b_cols);
  Matrix* result_2 = matrix_new(a_rows, b_cols);
  
  matrix_fill_random_flat(a);
  matrix_fill_random_flat(b);
  
  
  int i;
  
  timestamp_microsecs_t start_1 = timestamp_get_current_time();
  for(i=0;  i<num_tests; i++)
    matrix_multiply(a, b, result_1);
  timestamp_microsecs_t end_1 = timestamp_get_current_time();

  timestamp_microsecs_t start_2 = timestamp_get_current_time();
  for(i=0;  i<num_tests; i++)
    matrix_multiply_multithread(a, b, result_2);
  timestamp_microsecs_t end_2 = timestamp_get_current_time();
  
  //matrix_print(a);
  //matrix_print(b);
  //matrix_print(result_1);
  //matrix_print(result_2);

  if(matrix_is_pointwise_equal(result_1, result_2))
    fprintf(stderr, "equal\r\n");
  else
    fprintf(stderr, "not equal\r\n");
    
  fprintf(stderr, "speed_ratio: %02f\r\n", (double)(end_1-start_1) / (double)(end_2-start_2));
  
  return 0;
}

*/

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
