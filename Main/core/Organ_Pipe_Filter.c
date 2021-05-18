#include "Organ_Pipe_Filter.h"
#include "MKAiff.h"

#include <stdlib.h> //calloc
#include <pthread.h>

int organ_pipe_filter_init_filters(Organ_Pipe_Filter* self);
//empirically it takes about 5 or 6 frames from the time a
//note is turned on to show up in the audio, so
//delay by 4 and crossfade in the 5th and 6th frame
//must be at least 2
#define QUEUE_LENGTH 3

#define TEST_RECORD_MODE
#define TEST_RECORD_SECONDS 10

/*--------------------------------------------------------------------*/
struct Opaque_Organ_Pipe_Filter_Struct
{
  int           window_size;
  int           fft_N;
  int           fft_N_over_2;
  int           overlap;
  int           hop_size;
  int           return_size;
  int           sample_counter;
  int           input_index;
  dft_sample_t* window;
  dft_sample_t* running_input;
  dft_sample_t* running_output;
  dft_sample_t* real;
  dft_sample_t* imag;
  
  dft_sample_t* filters[OP_NUM_SOLENOIDS];
  dft_sample_t* noise;
  
  pthread_mutex_t note_amplitudes_mutex;
  float note_amplitudes[QUEUE_LENGTH][OP_NUM_SOLENOIDS];
  
  organ_pipe_filter_mode_t mode;
  double reduction_coefficient;
  double gate_thresh;
  double noise_cancel_thresh;
  
#if defined TEST_RECORD_MODE
  int did_save;
  MKAiff* test_input;
  MKAiff* test_output;
  int click_count;
#endif
};
  
/*--------------------------------------------------------------------*/
Organ_Pipe_Filter* organ_pipe_filter_new(int window_size /*power of 2 please*/, organ_pipe_filter_mode_t mode)
{
  Organ_Pipe_Filter* self = calloc(1, sizeof(*self));
  if(self != NULL)
    {
      int i;
      self->mode                 = mode;
      self->window_size          = window_size;
      
      if(self->mode <= ORGAN_PIPE_FILTER_MODE_RESYNTHESIZED_AUDIO)
        self->fft_N              = window_size;
      else
        self->fft_N              = 2 * window_size;
      
      self->fft_N_over_2         = self->fft_N >> 1;
      self->overlap              = 2;
      self->hop_size             = window_size / self->overlap;
      
      if(self->mode <= ORGAN_PIPE_FILTER_MODE_DFT)
        self->return_size        = self->fft_N_over_2;
      else
        self->return_size        = self->window_size;
      
      self->sample_counter       = 0;
      self->input_index          = 0;

      self->window               = calloc(self->window_size, sizeof(*(self->window)));
      self->running_input        = calloc(self->window_size, sizeof(*(self->running_input)));
      if(self->mode == ORGAN_PIPE_FILTER_MODE_RESYNTHESIZED_AUDIO)
        self->running_output     = calloc(self->fft_N      , sizeof(*(self->running_input)));
      self->real                 = calloc(self->fft_N      , sizeof(*(self->real)));
      self->imag                 = calloc(self->fft_N      , sizeof(*(self->imag)));

      if(self->window           == NULL) return organ_pipe_filter_destroy(self);
      if(self->running_input    == NULL) return organ_pipe_filter_destroy(self);
      if(self->mode == ORGAN_PIPE_FILTER_MODE_RESYNTHESIZED_AUDIO)
        if(self->running_output   == NULL) return organ_pipe_filter_destroy(self);
      if(self->real             == NULL) return organ_pipe_filter_destroy(self);
      if(self->imag             == NULL) return organ_pipe_filter_destroy(self);

      if(self->mode == ORGAN_PIPE_FILTER_MODE_RESYNTHESIZED_AUDIO)
        dft_init_half_sine_window(self->window, self->window_size);
      else
        dft_init_hann_window(self->window, self->window_size);

      self->noise = calloc(self->fft_N, sizeof(*(self->noise)));
      for(i=0; i<OP_NUM_SOLENOIDS; i++)
        {
          self->filters[i] = calloc(self->fft_N_over_2, sizeof(*(self->filters[i])));
          if(self->filters[i] == NULL)
            return organ_pipe_filter_destroy(self);
        }

      if(!organ_pipe_filter_init_filters(self))
        {
          fprintf(stderr, "Unable to find calibration samples for self-filterng. Try running opc first\r\n");
          return organ_pipe_filter_destroy(self);
        }

      self->reduction_coefficient  = ORGAN_PIPE_FILTER_DEFAULT_REDUCTION_COEFFICIENT;
      self->gate_thresh            = ORGAN_PIPE_FILTER_DEFAULT_GATE_THRESH;
      self->noise_cancel_thresh = ORGAN_PIPE_FILTER_DEFAULT_NOISE_CANCEL_THRESH;
    
#if defined TEST_RECORD_MODE
      self->test_input  = aiffWithDurationInSeconds(1, 44100, 16, TEST_RECORD_SECONDS+1);
      self->test_output = aiffWithDurationInSeconds(1, 44100, 16, TEST_RECORD_SECONDS+1);
#endif
    }
    
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
      if(self->noise != NULL)
        free(self->noise);
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
  int i, j;
  const char* home = getenv("HOME");
  char* filename_string;
  MKAiff* aiff;
  
  for(i=-1; i<OP_NUM_SOLENOIDS; i++)
    {
      int num_windows = 0;
      
      if(i<0)
        asprintf(&filename_string, "%s/%s/noise_sample.aiff", home, OP_PARAMS_DIR);
      else
        asprintf(&filename_string, "%s/%s/solenoid_%i_sample.aiff", home, OP_PARAMS_DIR, i);
      
      aiff = aiffWithContentsOfFile(filename_string);
      free(filename_string);
      if(aiff == NULL)
        return 0;
      
      dft_sample_t* filter = (i<0) ? self->noise : self->filters[i];
      
      for(;;)
        {
          memset(self->real, 0, self->fft_N * sizeof(*self->real));
          int samples_read = aiffReadFloatingPointSamplesAtPlayhead(aiff, self->real, self->window_size, aiffYes);
          if(samples_read < self->window_size)
            break;
          
          dft_apply_window(self->real, self->window, self->window_size);
          dft_real_forward_dft(self->real, self->imag, self->fft_N);
          dft_rect_to_polar(self->real, self->imag, self->fft_N_over_2);

          ++num_windows;

          if(num_windows == 1)
            memcpy(filter, self->real, self->fft_N_over_2 * sizeof(*self->real));
          else
            for(j=0; j<self->fft_N_over_2; j++)
              filter[j] += (self->real[j] - filter[j]) / (double)num_windows;
        }
      if(i>=0)
        for(j=0; j<self->fft_N_over_2; j++)
          {
            filter[j] -= self->noise[j];
            if(filter[j] < 0)
              filter[j] = 0;
          }
      
      aiff = aiffDestroy(aiff);
      if(num_windows == 0)
        return 0;
    }
  
  return 1;
}

/*--------------------------------------------------------------------*/
void organ_pipe_filter_notify_sounding_notes(Organ_Pipe_Filter* self, int sounding_notes[OP_NUM_SOLENOIDS])
{
  pthread_mutex_lock(&self->note_amplitudes_mutex);
  
  int i;
  //for(i=0; i<OP_NUM_SOLENOIDS; i++)
    //self->note_amplitudes[0][i] = sounding_notes[i] * 1/3.0;

  for(i=0; i<OP_NUM_SOLENOIDS; i++)
    self->note_amplitudes[0][i] = sounding_notes[i];
  
  self->click_count = 4;

  pthread_mutex_unlock(&self->note_amplitudes_mutex);
}

/*--------------------------------------------------------------------*/
void   organ_pipe_filter_set_reduction_coefficient(Organ_Pipe_Filter* self, double coeff)
{
  if(coeff < 0) coeff = 0;
  self->reduction_coefficient = coeff;
}

/*--------------------------------------------------------------------*/
double organ_pipe_filter_get_reduction_coefficient(Organ_Pipe_Filter* self)
{
  return self->reduction_coefficient;
}

/*--------------------------------------------------------------------*/
void   organ_pipe_filter_set_gate_thresh(Organ_Pipe_Filter* self, double thresh)
{
  if(thresh < 0) thresh = 0;
  self->gate_thresh = thresh;
}

/*--------------------------------------------------------------------*/
double organ_pipe_filter_get_gate_thresh(Organ_Pipe_Filter* self)
{
  return self->gate_thresh;
}

/*--------------------------------------------------------------------*/
void   organ_pipe_filter_set_noise_cancel_thresh(Organ_Pipe_Filter* self, double thresh)
{
  if(thresh < 0) thresh = 0;
  self->noise_cancel_thresh = thresh;
}

/*--------------------------------------------------------------------*/
double organ_pipe_filter_get_noise_cancel_thresh(Organ_Pipe_Filter* self)
{
  return self->noise_cancel_thresh;
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
            self->real[j] = self->running_input[(self->input_index+j) % self->window_size];
            
          for(j=self->window_size; j<self->fft_N; j++)
            self->real[j] = 0;

#if defined TEST_RECORD_MODE
          if(!self->did_save)
            aiffAppendFloatingPointSamples(self->test_input, self->real, self->hop_size, aiffFloatSampleType);
#endif

          dft_apply_window(self->real, self->window, self->window_size);
          dft_real_forward_dft(self->real, self->imag, self->fft_N);
          dft_rect_to_polar(self->real, self->imag, self->fft_N_over_2);

          pthread_mutex_lock(&self->note_amplitudes_mutex);

          for(k=1; k<self->fft_N_over_2; k++)
            self->real[k] -= self->noise[k];

          for(j=0; j<OP_NUM_SOLENOIDS; j++)
            {
              amplitude = self->note_amplitudes[QUEUE_LENGTH-1][j];
              //amplitude = self->note_amplitudes[QUEUE_LENGTH-1][j]
              //          + self->note_amplitudes[QUEUE_LENGTH-2][j]
              //          + self->note_amplitudes[QUEUE_LENGTH-3][j];
              //amplitude *= self->reduction_coefficient;
              if(amplitude > 0)
                //don't filter the DC offset
                for(k=1; k<self->fft_N_over_2; k++)
                  self->real[k] -= amplitude * self->filters[j][k];
            }

          for(j=QUEUE_LENGTH-1; j>0; j--)
            memcpy(self->note_amplitudes[j], self->note_amplitudes[j-1], OP_NUM_SOLENOIDS*sizeof(*self->note_amplitudes[j]));

          pthread_mutex_unlock(&self->note_amplitudes_mutex);

          //noise suppression
          float total_energy = 0;
          for(j=1; j<self->fft_N_over_2; j++)
            {
              //if(self->real[j] < 0.1)
              if(self->real[j] < self->noise_cancel_thresh)
                self->real[j] = 0;
              total_energy += self->real[j];
            }

          //noise gate
          //if(total_energy < 50)
          if(total_energy < self->gate_thresh)
            for(j=0; j<self->fft_N_over_2; j++)
              self->real[j] = 0;

          if(self->mode == ORGAN_PIPE_FILTER_MODE_AUTOCORRELATION)
            {
              for(j=0; j<self->fft_N_over_2; j++)
                {
                  self->real[j] *= self->real[j];
                  self->imag[j] = 0;
                }
              dft_polar_to_rect(self->real, self->imag, self->fft_N_over_2);
              dft_real_inverse_dft(self->real, self->imag, self->fft_N);
            }
          else if(self->mode == ORGAN_PIPE_FILTER_MODE_RESYNTHESIZED_AUDIO)
            {
              dft_polar_to_rect(self->real, self->imag, self->fft_N_over_2);
              dft_real_inverse_dft(self->real, self->imag, self->fft_N);
              dft_apply_window(self->real, self->window, self->window_size);
              
              for(j=0; j<self->window_size-self->hop_size; j++)
                {
                  self->real[j] += self->running_output[j+self->hop_size];
                  self->running_output[j] = self->real[j];
                }
              for(; j<self->window_size; j++)
                self->running_output[j] = self->real[j];
#if defined TEST_RECORD_MODE
              if(!self->did_save)
                {
                  if(self->click_count > 0)
                    {
                      --self->click_count;
                      self->real[0] = 1.0;
                      
                    }
                  aiffAddFloatingPointSamplesAtPlayhead(self->test_output, self->real, self->hop_size, aiffFloatSampleType, aiffYes);
                }
#endif
            }

#if defined TEST_RECORD_MODE
          if(!self->did_save)
            if(aiffDurationInSeconds(self->test_input) > TEST_RECORD_SECONDS)
              {
                 aiffSaveWithFilename(self->test_input, "test_input.aiff");
                 aiffSaveWithFilename(self->test_output, "test_output.aiff");
                 self->did_save = 1;
                 fprintf(stderr, "saved\r\n");
              }
#endif
          onprocess(onprocess_self, self->real, self->return_size);
        }
    }
}
