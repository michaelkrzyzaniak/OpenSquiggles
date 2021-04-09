#include "Organ_Pipe_Filter.h"
#include "../extras/MKAiff.h"

#include <stdlib.h> //calloc
#include <pthread.h>

int organ_pipe_filter_init_filters(Organ_Pipe_Filter* self);

/*--------------------------------------------------------------------*/
struct Opaque_Organ_Pipe_Filter_Struct
{
  int           window_size;
  int           fft_N;
  int           overlap;
  int           hop_size;
  int           sample_counter;
  int           input_index;
  dft_sample_t* window;
  dft_sample_t* running_input;
  dft_sample_t* running_output;
  dft_sample_t* real;
  dft_sample_t* imag;
  
  dft_sample_t* filters[OP_NUM_SOLENOIDS];
  
  pthread_mutex_t note_amplitudes_mutex;
  float note_amplitudes_private_a[OP_NUM_SOLENOIDS];
  float note_amplitudes_private_b[OP_NUM_SOLENOIDS];
  float* note_amplitudes_newer;
  float* note_amplitudes_older;
  
  int did_save;
  MKAiff* test_input;
  MKAiff* test_output;
};

/*--------------------------------------------------------------------*/
Organ_Pipe_Filter* organ_pipe_filter_new(int window_size /*power of 2 please*/)
{
  Organ_Pipe_Filter* self = calloc(1, sizeof(*self));
  if(self != NULL)
    {
      int i;
      
      self->window_size          = window_size;
      self->fft_N                = 2 * window_size;
      
      self->overlap              = 2;

      self->hop_size             = window_size / self->overlap;
      self->sample_counter       = 0;

      self->input_index          = 0;

      self->window               = calloc(self->window_size, sizeof(*(self->window)));
      self->running_input        = calloc(self->window_size, sizeof(*(self->running_input)));
      self->running_output       = calloc(self->fft_N      , sizeof(*(self->running_input)));
      self->real                 = calloc(self->fft_N      , sizeof(*(self->real)));
      self->imag                 = calloc(self->fft_N      , sizeof(*(self->imag)));

      if(self->window           == NULL) return organ_pipe_filter_destroy(self);
      if(self->running_input    == NULL) return organ_pipe_filter_destroy(self);
      if(self->running_output   == NULL) return organ_pipe_filter_destroy(self);
      if(self->real             == NULL) return organ_pipe_filter_destroy(self);
      if(self->imag             == NULL) return organ_pipe_filter_destroy(self);

      //half sine for reconstruction
      dft_init_half_sine_window(self->window, self->window_size);
      //dft_init_hamming_window(self->window, self->window_size);
      
      for(i=0; i<OP_NUM_SOLENOIDS; i++)
        {
          self->filters[i] = calloc(self->fft_N, sizeof(*(self->filters[i])));
          if(self->filters[i] == NULL)
            return organ_pipe_filter_destroy(self);
        }
    }
  
  if(!organ_pipe_filter_init_filters(self))
    return organ_pipe_filter_destroy(self);
  
  self->note_amplitudes_newer = self->note_amplitudes_private_a;
  self->note_amplitudes_older = self->note_amplitudes_private_b;
  
  self->test_input  = aiffWithDurationInSeconds(1, 44100, 16, 120);
  self->test_output = aiffWithDurationInSeconds(1, 44100, 16, 120);
  
  
  
  return self;
}

/*--------------------------------------------------------------------*/
Organ_Pipe_Filter* organ_pipe_filter_destroy(Organ_Pipe_Filter* self)
{
  if(self != NULL)
    {
      if(self->window != NULL)
        free(self->window);
      if(self->running_input != NULL)
        free(self->running_input);
      if(self->running_output != NULL)
        free(self->running_output);
      if(self->real != NULL)
        free(self->real);
      if(self->imag != NULL)
        free(self->imag);
      int i;
      for(i=0; i<OP_NUM_SOLENOIDS; i++)
        {
          if(self->filters[i] != NULL)
            free(self->filters[i]);
        }
      free(self);
    }
  return (Organ_Pipe_Filter*) NULL;
}

/*--------------------------------------------------------------------*/
int organ_pipe_filter_init_filters(Organ_Pipe_Filter* self)
{
  int i;
  const char* home = getenv("HOME");
  char* filename_string;
  MKAiff* aiff;
  for(i=0; i<OP_NUM_SOLENOIDS; i++)
    {
      asprintf(&filename_string, "%s/%s/solenoid_%i_sample.aiff", home, OP_PARAMS_DIR, i);
      aiff = aiffWithContentsOfFile(filename_string);
      free(filename_string);
      if(aiff == NULL)
        return 0;
      
      int n = aiffReadFloatingPointSamplesAtPlayhead(aiff, self->filters[i], self->window_size, aiffYes);
      if(n < self->window_size)
        {aiffDestroy(aiff); return -1;}

      dft_apply_window(self->filters[i], self->window, self->window_size);
      dft_real_forward_dft(self->filters[i], self->imag, self->fft_N);
      dft_rect_to_polar(self->filters[i], self->imag, self->fft_N);
  
      aiffDestroy(aiff);
    }
  return 1;
}

/*--------------------------------------------------------------------*/
void organ_pipe_filter_notify_sounding_notes(Organ_Pipe_Filter* self, int sounding_notes[OP_NUM_SOLENOIDS])
{
  pthread_mutex_lock(&self->note_amplitudes_mutex);
  
  //assumes 50% overlapping windows
  int i;
  for(i=0; i<OP_NUM_SOLENOIDS; i++)
    self->note_amplitudes_newer[i] = sounding_notes[i] * 0.5;
  
  pthread_mutex_unlock(&self->note_amplitudes_mutex);
}

/*--------------------------------------------------------------------*/
void organ_pipe_filter_process(Organ_Pipe_Filter* self, dft_sample_t* real_input, int len, organ_pipe_filter_onprocess_t onprocess, void* onprocess_self)
{
  int i, j, k;
  float amplitude;

  for(i=0; i<len; i++)
    {
      self->running_input[self->input_index++] = real_input[i];
      self->input_index %= self->window_size;
    
      if(++self->sample_counter == self->hop_size)
        {
          self->sample_counter = 0;
        
          for(j=0; j<self->window_size; j++)
            {
              self->real[j] = self->running_input[(self->input_index+j) % self->window_size];
              self->imag[j] = 0;
            }
          for(j=self->window_size; j<self->fft_N; j++)
            {
              self->real[j] = 0;
              self->imag[j] = 0;
            }
          
          if(!self->did_save)
            aiffAppendFloatingPointSamples(self->test_input, self->real, self->hop_size, aiffFloatSampleType);
            
          dft_apply_window(self->real, self->window, self->window_size);
          dft_real_forward_dft(self->real, self->imag, self->fft_N);

          dft_rect_to_polar(self->real, self->imag, self->window_size);
  
          pthread_mutex_lock(&self->note_amplitudes_mutex);
          //assumes 50% overlapping windows and self->window_size % len == 0
          //assumes 50% overlapping windows and self->window_size % len == 0
          
          for(j=0; j<OP_NUM_SOLENOIDS; j++)
            {
              amplitude = self->note_amplitudes_older[j] + self->note_amplitudes_newer[j];
              if(amplitude > 0)
                for(k=0; k<self->window_size; k++)
                  self->real[k] -= amplitude * self->filters[j][k];
            }

          for(j=0; j<OP_NUM_SOLENOIDS; j++)
            self->note_amplitudes_older[j] = self->note_amplitudes_newer[j];
           
          //float* temp = self->note_amplitudes_older;
          //self->note_amplitudes_older = self->note_amplitudes_newer;
          //self->note_amplitudes_newer = temp;
          pthread_mutex_unlock(&self->note_amplitudes_mutex);
          
          for(j=0; j<self->window_size; j++)
            if(self->real[j] < 0)
              self->real[j] = 0;

          dft_polar_to_rect(self->real, self->imag, self->window_size);
  
          //dft_real_autocorrelate(self->real, self->imag, self->fft_N);

          dft_real_inverse_dft(self->real, self->imag, self->fft_N);
          dft_apply_window(self->real, self->window, self->window_size);
          for(j=self->window_size; j<self->fft_N; j++)
            self->real[j] = 0;
          

          for(j=0; j<self->window_size-self->hop_size; j++)
            self->running_output[j] = self->real[j] + self->running_output[j+self->hop_size];

          for(; j<self->window_size; j++)
            self->running_output[j] = self->real[j];
 
          if(!self->did_save)
            aiffAddFloatingPointSamplesAtPlayhead(self->test_output, self->running_output, self->hop_size, aiffFloatSampleType, aiffYes);
            
          if(!self->did_save)
            if(aiffDurationInSeconds(self->test_input) > 26)
              {
                 aiffSaveWithFilename(self->test_input, "test_input.aiff");
                 aiffSaveWithFilename(self->test_output, "test_output.aiff");
                 self->did_save = 1;
                 fprintf(stderr, "saved\r\n");
              }
          
          onprocess(onprocess_self, self->real, self->fft_N / 2);
        }
    }
}
