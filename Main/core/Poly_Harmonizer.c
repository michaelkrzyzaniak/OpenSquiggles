/*------------------------------------------------------------------------
 _  _                         _
| || |__ _ _ _ _ __  ___ _ _ (_)______ _ _  __
| __ / _` | '_| '  \/ _ \ ' \| |_ / -_) '_|/ _|
|_||_\__,_|_| |_|_|_\___/_||_|_/__\___|_|(_)__|
------------------------------------------------------------------------
Written by Michael Krzyzaniak

----------------------------------------------------------------------*/
#include "Poly_Harmonizer.h"

#include <stdlib.h> //calloc
#include <math.h>  //log, exp, etc
//#include <pthread.h>  //thread, mutex

#include "Poly_Harmonizer.h"

//#define WINDOW_SIZE 4096
#define WINDOW_SIZE 2048
#define NUM_BANDS   30

#define MAX_MAX_POLYPHONY 20
#define MAX_MAX_NOTE_RANGE 127

#define MIDI2CPS(x)  (440 * pow(2, ((x)-69) / 12.0))
#define CPS2MIDI(x) ((int)round(69 + 12.0 * log2((x) / 440.0)))

void poly_harmonizer_init_band_center_freqs(Poly_Harmonizer* self);
void poly_harmonizer_stft_process_callback(void* SELF, dft_sample_t* real, int N);

/*--------------------------------------------------------------------*/
struct opaque_poly_harmonizer_struct
{
  unsigned prev_note;

  void* notes_changed_callback_self;
  poly_harmonizer_notes_changed_callback_t notes_changed_callback;
  
  Organ_Pipe_Filter*   filter;
  
  double sample_rate;
  double tau_min;
  double tau_max;
  int min_note;
  int max_note;
  int note_range;
  int max_polyphony;
  double resolution;
  int M;
  int K;
  double alpha;
  double beta;
  double delta;
  double gamma;
  
  int band_center_freq_indices[NUM_BANDS+2];
  
  int on_for;
  int off_for;
  int on_count[MAX_MAX_NOTE_RANGE];
  int prev_notes[MAX_MAX_POLYPHONY];
  int num_prev_notes;
  
  //pthread_mutex_t clear_mutex;
};

/*-----------------------------------------------------------------------*/
Poly_Harmonizer* poly_harmonizer_new(double sample_rate)
{
  Poly_Harmonizer* self = calloc(1, sizeof(*self));

  if(self != NULL)
    {
      self->sample_rate = sample_rate;
      
      self->filter = organ_pipe_filter_new(WINDOW_SIZE, ORGAN_PIPE_FILTER_MODE_PADDED_DFT);
      if(self->filter == NULL)
        return poly_harmonizer_destroy(self);
      
      //fft bins: tau_min: 195.0 tau_max: 3.7; (self->K / tau)
      //periods: tau_min: 21.0 tau_max: 1102.5;
      self->tau_min  = self->sample_rate / MIDI2CPS(self->max_note);
      self->tau_max  = self->sample_rate / MIDI2CPS(self->min_note);
      poly_harmonizer_set_resolution(self, POLY_HARMONIZER_DEFAULT_RESOLUTION);
      self->min_note = POLY_HARMONIZER_DEFAULT_MIN_NOTE;
      self->max_note = POLY_HARMONIZER_DEFAULT_MAX_NOTE;
      self->note_range = self->max_note - self->min_note;
      self->max_polyphony = POLY_HARMONIZER_DEFAULT_MAX_POLYPHONY;
      self->M = POLY_HARMONIZER_DEFAULT_M;
      self->K = 2*WINDOW_SIZE;
      self->alpha = 27;
      self->beta  = 230;
      self->delta = POLY_HARMONIZER_DEFAULT_DELTA;
      self->gamma = POLY_HARMONIZER_DEFAULT_GAMMA;
      
      self->on_for  = POLY_HARMONIZER_DEFAULT_ON_FOR;
      self->off_for = POLY_HARMONIZER_DEFAULT_OFF_FOR;
      
      if(WINDOW_SIZE == 4096)
        {
          self->alpha = 52;
          self->delta = POLY_HARMONIZER_DEFAULT_DELTA; //0.89
        }
      
      poly_harmonizer_init_band_center_freqs(self);
    }
  return self;
}

/*-----------------------------------------------------------------------*/
Poly_Harmonizer*       poly_harmonizer_destroy               (Poly_Harmonizer* self)
{
  if(self != NULL)
    {
      self->filter = organ_pipe_filter_destroy(self->filter);

      free(self);
    }
  return (Poly_Harmonizer* ) NULL;
}

/*-----------------------------------------------------------------------*/
void poly_harmonizer_init_band_center_freqs(Poly_Harmonizer* self)
{
  int b;
  double c_b;
  //this ends up ranging from 26.0 Hz to 6935 Hz
  for(b=0; b<NUM_BANDS+2; b++)
    {
      c_b = 229 * (pow(10, (b+1) / 21.4) - 1);
      c_b = dft_bin_of_frequency(c_b, self->sample_rate, self->K);

      //todo: could get better low-freq by not rounding and doing proper interpolation
      self->band_center_freq_indices[b] = round(c_b);
      if(self->band_center_freq_indices[b] < 1)
        self->band_center_freq_indices[b] = 1;
      if(self->band_center_freq_indices[b] >= WINDOW_SIZE)
        self->band_center_freq_indices[b] = WINDOW_SIZE-1;
    }
}

/*-----------------------------------------------------------------------*/
void poly_harmonizer_set_notes_changed_callback(Poly_Harmonizer* self, poly_harmonizer_notes_changed_callback_t callback,  void* callback_self)
{
  self->notes_changed_callback = callback;
  self->notes_changed_callback_self = callback_self;
}

/*-----------------------------------------------------------------------*/
void poly_harmonizer_init_state(Poly_Harmonizer* self)
{
  //pthread_mutex_lock(&self->clear_mutex);

  //pthread_mutex_unlock(&self->clear_mutex);
}

/*-----------------------------------------------------------------------*/
Organ_Pipe_Filter* poly_harmonizer_get_organ_pipe_filter(Poly_Harmonizer* self)
{
  return self->filter;
}

/*-----------------------------------------------------------------------*/
void    poly_harmonizer_set_on_for (Poly_Harmonizer* self, int on_for)
{
  if(on_for < 1) on_for = 1;
  self->on_for = on_for;
}

/*-----------------------------------------------------------------------*/
int     poly_harmonizer_get_on_for (Poly_Harmonizer* self)
{
  return self->on_for;
}

/*-----------------------------------------------------------------------*/
void    poly_harmonizer_set_off_for(Poly_Harmonizer* self, int off_for)
{
  if(off_for < 1) off_for = 1;
  self->off_for = off_for;
}

/*-----------------------------------------------------------------------*/
int     poly_harmonizer_get_off_for(Poly_Harmonizer* self)
{
  return self->off_for;
}

/*-----------------------------------------------------------------------*/
void    poly_harmonizer_set_min_note(Poly_Harmonizer* self, double min_note)
{
  if(min_note < 0)
    min_note = 0;
  if(min_note > self->max_note)
    min_note = self->max_note;
  if((self->max_note - min_note) > MAX_MAX_NOTE_RANGE)
    min_note = self->max_note - MAX_MAX_NOTE_RANGE;
  self->min_note = min_note;
  self->note_range = self->max_note - self->min_note;
}

/*-----------------------------------------------------------------------*/
double  poly_harmonizer_get_min_note(Poly_Harmonizer* self)
{
  return self->min_note;
}

/*-----------------------------------------------------------------------*/
void    poly_harmonizer_set_max_note(Poly_Harmonizer* self, double max_note)
{
  if(max_note < self->min_note)
    max_note = self->min_note;
  if(max_note > 127)
    max_note = 127;
  if((max_note - self->min_note) > MAX_MAX_NOTE_RANGE)
    max_note = self->min_note + MAX_MAX_NOTE_RANGE;
  self->max_note = max_note;
  self->note_range = self->max_note - self->min_note;
}

/*-----------------------------------------------------------------------*/
double  poly_harmonizer_get_max_note(Poly_Harmonizer* self)
{
  return self->max_note;
}

/*-----------------------------------------------------------------------*/
void    poly_harmonizer_set_resolution(Poly_Harmonizer* self, double divisions_per_half_step)
{
  if(divisions_per_half_step <= 1) divisions_per_half_step = 1;
  
  self->resolution = 1.0/divisions_per_half_step;
}

/*-----------------------------------------------------------------------*/
double  poly_harmonizer_get_resolution(Poly_Harmonizer* self)
{
  return 1.0/self->resolution;
}

/*-----------------------------------------------------------------------*/
void    poly_harmonizer_set_max_polyphony(Poly_Harmonizer* self, int polyphony)
{
  if(polyphony < 0) polyphony = 0;
  if(polyphony > MAX_MAX_POLYPHONY) polyphony = MAX_MAX_POLYPHONY;
  self->max_polyphony = polyphony;
}

/*-----------------------------------------------------------------------*/
int     poly_harmonizer_get_max_polyphony(Poly_Harmonizer* self)
{
  return self->max_polyphony;
}

/*-----------------------------------------------------------------------*/
void    poly_harmonizer_set_delta(Poly_Harmonizer* self, double delta)
{
  if(delta < 0) delta = 0;
  //if(delta > 1) delta = 1;
  
  self->delta = delta;
}

/*-----------------------------------------------------------------------*/
double  poly_harmonizer_get_delta(Poly_Harmonizer* self)
{
  return self->delta;
}

/*-----------------------------------------------------------------------*/
void    poly_harmonizer_set_gamma(Poly_Harmonizer* self, double gamma)
{
  self->gamma = gamma;
}

/*-----------------------------------------------------------------------*/
double  poly_harmonizer_get_gamma(Poly_Harmonizer* self)
{
  return self->gamma;
}

/*-----------------------------------------------------------------------*/
void    poly_harmonizer_set_M(Poly_Harmonizer* self, int M)
{
  if(M<1) M=1;
  self->M = M;
}

/*-----------------------------------------------------------------------*/
int     poly_harmonizer_get_M(Poly_Harmonizer* self)
{
  return self->M;
}

/*-----------------------------------------------------------------------*/
void poly_harmonizer_process_audio(Poly_Harmonizer* self, auSample_t* buffer, int num_frames)
{
  organ_pipe_filter_process(self->filter, buffer, num_frames, poly_harmonizer_stft_process_callback, self);
}

/*-----------------------------------------------------------------------*/
//whitens magnitude from indices self->band_center_freq_indices[1] to self->band_center_freq_indices[NUM_BANDS] inclusive
//k=5 to k=576, inclusive for WINDOW_SIZE 2048
//k=10 to k=1153 WINDOW_SIZE 4096
void poly_harmonizer_spectral_whitening(Poly_Harmonizer* self, dft_sample_t* magnitude, int N)
{
  int k, b;
  double bands[NUM_BANDS] = {0};
  double step;
  double H_b_k;
  int* centers = self->band_center_freq_indices;
  double nu_minus_1 = 0.33 - 1;
  double gamma_k;
  
  //fprintf(stderr, "k=%i to k=%i\r\n", self->band_center_freq_indices[1], self->band_center_freq_indices[NUM_BANDS]);
  
  for(b=0; b<NUM_BANDS; b++)
    {
      //rising side of triangular window
      step = 1.0 / (double)(centers[b+1] - centers[b]);
      H_b_k = 0;
      for(k=centers[b]; k<centers[b+1]; k++)
        {
          //first coefficient is H_b_k but it is left for clarity
          bands[b] += H_b_k * magnitude[k];
          H_b_k += step;
        }
      
      //falling side of triangular window
      step = 1.0 / (double)(centers[b+2] - centers[b+1]);
      H_b_k = 1;
      for(k=centers[b+1]; k<centers[b+2]; k++)
        {
          //todo: optimize by pre-computing magnitude[k] * magnitude[k]
          bands[b] += H_b_k * magnitude[k] * magnitude[k];
          H_b_k -= step;
        }
      
      //rest of equation 2
      bands[b] /= (double)(self->K);
      if(bands[b] > 0)
        {
          bands[b] = sqrt(bands[b]);
          //compression coefficients
          bands[b] = pow(bands[b], nu_minus_1);
        }
    }

  for(b=0; b<NUM_BANDS-1; b++)
    {
      step = (bands[b+1] - bands[b]) / (double)(centers[b+2] - centers[b+1]);
      gamma_k = bands[b];
      for(k=centers[b+1]; k<centers[b+2]; k++)
        {
          magnitude[k] *= gamma_k;
          gamma_k += step;
        }
    }
  magnitude[k] *= bands[b];
}

/*-----------------------------------------------------------------------*/
double poly_harmonizer_salience(Poly_Harmonizer* self, dft_sample_t* Y, double tau_low, double tau_up)
{
  double tau = (tau_low + tau_up) / 2.0;
  double delta_tau = tau_up - tau_low;
  double f = self->sample_rate;
  double f_over_tau_low_plus_alpha = (f / tau_low) + self->alpha;
  double f_over_tau_up = f / tau_up;
  double g_tau_m;
  
  int m, k;
  int k_min, k_max;
  double max_Y_k;
  
  double K_over_tau_plus_delta_tau_over_2  = self->K / (tau + (delta_tau / 2.0));
  double K_over_tau_minus_delta_tau_over_2 = self->K / (tau - (delta_tau / 2.0));

  double sum = 0;

  for(m=1; m<=self->M; m++)
    {
      g_tau_m = f_over_tau_low_plus_alpha / (m * f_over_tau_up + self->beta);
      
      max_Y_k = 0;
      k_min = round(m * K_over_tau_plus_delta_tau_over_2);
      k_max = round(m * K_over_tau_minus_delta_tau_over_2);
      
      if(k_min < self->band_center_freq_indices[1])
        k_max = self->band_center_freq_indices[1];
      if(k_max > self->band_center_freq_indices[NUM_BANDS])
        k_max = self->band_center_freq_indices[NUM_BANDS];

      //fprintf(stderr, "\tm: %i\tk_min: %i\t k_max: %i\r\n", m, k_min-self->band_center_freq_indices[1], k_max-self->band_center_freq_indices[1]);

      for(k=k_min; k<=k_max; k++)
        if(Y[k] > max_Y_k)
          max_Y_k = Y[k];
      
      sum += g_tau_m * max_Y_k;
    }

  return sum;
}

/*-----------------------------------------------------------------------*/
double poly_harmonizer_get_max_salience(Poly_Harmonizer* self, dft_sample_t* Y, double* returned_salience)
{
  double tau_low_Q = self->tau_min;
  double tau_low_q_best = tau_low_Q;
  double tau_up_Q = self->tau_max;
  double tau_up_q_best = tau_up_Q;
  double s_max_Q;
  double s_max_q_best;
  double max_salience = 0;
  
  while((tau_up_q_best - tau_low_q_best) > self->resolution)
    {
      tau_low_Q = (tau_low_q_best + tau_up_q_best) / 2.0;
      tau_up_Q = tau_up_q_best;
      tau_up_q_best = tau_low_Q;
      s_max_q_best = poly_harmonizer_salience(self, Y, tau_low_q_best, tau_up_q_best);
      s_max_Q      = poly_harmonizer_salience(self, Y, tau_low_Q     , tau_up_Q     );
      
      if(s_max_Q > s_max_q_best)
        {
          tau_low_q_best = tau_low_Q;
          tau_up_q_best = tau_up_Q;
          max_salience = s_max_Q;
        }
      else
        max_salience = s_max_q_best;
    }
  
  *returned_salience = max_salience;
  return (tau_low_q_best + tau_up_q_best) / 2.0;
}

/*-----------------------------------------------------------------------*/
double poly_harmonizer_get_max_salience_direct(Poly_Harmonizer* self, dft_sample_t* Y, double* returned_salience)
{
  double salience;
  double hz, tau, bin;
  double i;
  int m, k;

  double max_tau = 0;
  int    max_salience = 0;

  double g_tau_m;
  double f_over_tau_plus_alpha;
  
  for(i=self->min_note; i<=self->max_note; i+=self->resolution)
    {
      salience = 0;
      //hz could be pre-computed;
      hz = MIDI2CPS(i);
      tau = self->sample_rate / hz;
      bin = hz * (self->K / (double)self->sample_rate);

      double f_over_tau_plus_alpha = hz + self->alpha;
      
      for(m=1; m<=self->M; m++)
        {
          g_tau_m = f_over_tau_plus_alpha / (m * hz + self->beta);
          k = round(m * bin);
          
          if((k >= self->band_center_freq_indices[1]) && (k <= self->band_center_freq_indices[NUM_BANDS]))
            salience += g_tau_m * Y[k];
          else break;
        }
     
      if(salience >= max_salience)
        {
          max_tau      = tau;
          max_salience = salience;
        }
    }
  
  *returned_salience = max_salience;
  return max_tau;
}

/*-----------------------------------------------------------------------*/
int poly_harmonizer_iterative_estimation_and_cancellation(Poly_Harmonizer* self, dft_sample_t* Y, float* freqs, int max_num_freqs)
{
  int i;
  double max_salience;
  double delta_tau_over_2 = 0.5 / 2.0;
  double f = self->sample_rate;
  double g_tau_m;
  int m, k;
  int k_min, k_max;
  double equation_8_sum = 0;
  double prev_S_j = 0;
            
  for(i=0; i<max_num_freqs; i++)
    {
      //organ : MIDI 68    415 Hz    tau 106.3    bin 69
      //guitar: MIDI 64    330 Hz    tau 133.6    bin 53
      
    
      //fprintf(stderr, "tau: %f\tm:%i\tk_min:%i\tk_max:%i\r\n", tau, m, k_min, k_max);
/*
  fprintf(stderr, "-------------------------------------\r\n");
    for(k=self->band_center_freq_indices[1]; k<=self->band_center_freq_indices[NUM_BANDS]; k++)
      {
        //fprintf(stderr, "%f\t%f\r\n", dft_frequency_of_bin(k, self->sample_rate, self->K), Y[k]);
        fprintf(stderr, "%f\r\n", Y[k]);
     }
*/
      double tau = poly_harmonizer_get_max_salience_direct(self, Y, &max_salience);
      
      if((i==0) && (round(tau) == self->tau_min))
        return 0;
        
      double f_over_tau = f / tau;
      double f_over_tau_plus_alpha = f_over_tau + self->alpha;
      //double K_over_tau_plus_delta_tau_over_2  = self->K / (tau + delta_tau_over_2);
      //double K_over_tau_minus_delta_tau_over_2 = self->K / (tau - delta_tau_over_2);
      double S_j;
        
      for(m=1; m<=self->M; m++)
        {
          g_tau_m = f_over_tau_plus_alpha / (m * f_over_tau + self->beta);
          k_min = round(m * self->K / tau);
          
          //search for local maximum
          while(k_min > self->band_center_freq_indices[1])
            if(Y[k_min] <= Y[k_min-1])
              --k_min;
            else
              break;
              
          while(k_min < self->band_center_freq_indices[NUM_BANDS])
            if(Y[k_min] <= Y[k_min+1])
              ++k_min;
            else
              break;

          //search minima on both sides of maximum
           k_max = k_min;
          while(k_min > self->band_center_freq_indices[1])
            if(Y[k_min] >= Y[k_min-1])
              --k_min;
            else
              break;
              
          while(k_max < self->band_center_freq_indices[NUM_BANDS])
            if(Y[k_max] >= Y[k_max+1])
              ++k_max;
            else
              break;

/*
          k_min = round(m * K_over_tau_plus_delta_tau_over_2);
          k_max = round(m * K_over_tau_minus_delta_tau_over_2);
*/
          
          if(k_min < self->band_center_freq_indices[1])
            k_max = self->band_center_freq_indices[1];
          if(k_max > self->band_center_freq_indices[NUM_BANDS])
            k_max = self->band_center_freq_indices[NUM_BANDS];

          for(k=k_min; k<=k_max; k++)
            {
              Y[k] *= self->delta;
              //Y[k] *= g_tau_m * self->delta;
              //Y[k] -= Y[k] * g_tau_m * self->delta;
              if(Y[k] < 0) Y[k] = 0;
            }
        }
       
      equation_8_sum += max_salience;
      S_j = equation_8_sum / (pow(i+1, self->gamma));
      
      if(S_j <= prev_S_j)
        break;

      prev_S_j = S_j;
  
      freqs[i] = self->sample_rate / tau;
    }

  return i;
}

/*-----------------------------------------------------------------------*/
int poly_harmonizer_array_contains_int(Poly_Harmonizer* self, int* arr, int N, int contains)
{
  int i;
  for(i=0; i<N; i++)
    if(arr[i] == contains)
      return 1;
  
  return 0;
}

/*-----------------------------------------------------------------------*/
void poly_harmonizer_stft_process_callback(void* SELF, dft_sample_t* magnitude, int N)
{
  Poly_Harmonizer* self = SELF;
  if(N != WINDOW_SIZE) return;

  int i, k;
  float freqs[self->max_polyphony];
  int current_notes[self->max_polyphony];
  int num_current_notes=0;
  int many_hot_output[MAX_MAX_NOTE_RANGE] = {0};
  
  poly_harmonizer_spectral_whitening(self, magnitude, N);

  int num_notes = poly_harmonizer_iterative_estimation_and_cancellation(self, magnitude, freqs, self->max_polyphony);
 
 
  for(i=0; i<num_notes; i++)
    {
      int note = CPS2MIDI(freqs[i]);
      if((note < self->min_note) || (note > self->max_note))
        continue;
      
      many_hot_output[note - self->min_note] = 1;
    }
 
  for(i=0; i<self->note_range; i++)
    {
      if(many_hot_output[i])
        {
          self->on_count[i] += 1;
          if(self->on_count[i] > self->on_for)
            self->on_count[i] = self->on_for;
            
          if((self->on_count[i] == self->on_for) || poly_harmonizer_array_contains_int(self, self->prev_notes, self->num_prev_notes, i + self->min_note))
            if(num_current_notes<self->note_range-1)
              current_notes[num_current_notes++] = i + self->min_note;
        }
      else
        {
          self->on_count[i] -= 1;
          if(self->on_count[i] <= 0)
            self->on_count[i] = 0;
          else if(poly_harmonizer_array_contains_int(self, self->prev_notes, self->num_prev_notes, i + self->min_note))
            if(num_current_notes<self->note_range-1)
              current_notes[num_current_notes++] = i + self->min_note;
        }
    }
 
  int notes_did_change = self->num_prev_notes != num_current_notes;
  for(i=0; i<num_current_notes; i++)
    {
      if(current_notes[i] != self->prev_notes[i])
        notes_did_change = 1;
      self->prev_notes[i] = current_notes[i];
    }
  self->num_prev_notes = num_current_notes;
  
  if(notes_did_change)
    {
      fprintf(stderr, "%i notes: {", num_current_notes);
      for(i=0; i<num_current_notes; i++)
        fprintf(stderr, " %i ", current_notes[i]);
      fprintf(stderr, "}\r\n");
 
 
      if(self->notes_changed_callback != NULL)
        self->notes_changed_callback(self->notes_changed_callback_self, current_notes, num_current_notes);
    }

}
