/*------------------------------------------------------------------------
 _  _                         _
| || |__ _ _ _ _ __  ___ _ _ (_)______ _ _  __
| __ / _` | '_| '  \/ _ \ ' \| |_ / -_) '_|/ _|
|_||_\__,_|_| |_|_|_\___/_||_|_/__\___|_|(_)__|
------------------------------------------------------------------------
Written by Michael Krzyzaniak

Version 2.0 March 21 2021
multi-thread support and many small changes
----------------------------------------------------------------------*/
#include <stdlib.h> //calloc
#include <math.h>  //log, exp, etc
#include <pthread.h>  //thread, mutex

#include "LSTM_Harmonizer.h"
#define MAXNUM_NOTES 10
#define NUM_BANDS   30
#define WINDOW_SIZE 2048
#define K           (2*WINDOW_SIZE)

void lstm_harmonizer_init_band_center_freqs(LSTM_Harmonizer* self);
void lstm_harmonizer_stft_process_callback(void* SELF, dft_sample_t* real, int N);

/*--------------------------------------------------------------------*/
struct opaque_lstm_harmonizer_struct
{
  unsigned num_audio_inputs;
  unsigned num_outputs;
  unsigned num_layer_1_inputs;
  unsigned num_layer_2_inputs;
  unsigned num_layer_3_inputs;
  unsigned lowest_midi_note;
  unsigned prev_note_out;
  //int      prev_notes_out[MAXNUM_NOTES];
  //unsigned num_prev_notes_out;
  unsigned note_timer;
  int band_center_freq_indices[NUM_BANDS+2];
  
  void* notes_changed_callback_self;
  lstm_harmonizer_notes_changed_callback_t notes_changed_callback;
  
  Organ_Pipe_Filter*   filter;
  
  Matrix* audio_features;
  Matrix* input_vector;
  
  Matrix* layer_1_weights;
  Matrix* layer_1_biases;
  Matrix* layer_1_out;
  
  Matrix* lstm_Wf;
  Matrix* lstm_Wi;
  Matrix* lstm_Wo;
  Matrix* lstm_Wg;
  Matrix* lstm_Uf;
  Matrix* lstm_Ui;
  Matrix* lstm_Uo;
  Matrix* lstm_Ug;
  Matrix* lstm_bf;
  Matrix* lstm_bi;
  Matrix* lstm_bo;
  Matrix* lstm_bg;
  
  Matrix* lstm_h_t;
  Matrix* lstm_c_t;

  Matrix* lstm_f_t;
  Matrix* lstm_i_t;
  Matrix* lstm_o_t;
  Matrix* lstm_g_t;
  
  Matrix* lstm_temp_w;
  Matrix* lstm_temp_u;

  Matrix* layer_3_weights;
  Matrix* layer_3_biases;
  Matrix* layer_3_out;
  
  //Matrix* autoregressive_histogram;
  //matrix_val_t histogram_coeff;
  
  pthread_mutex_t clear_mutex;
};

/*-----------------------------------------------------------------------*/
char* lstm_harmonizer_filename(char* folder, char* filename, char* buffer)
{
  sprintf(buffer, "%s/%s", folder, filename);
  return buffer;
}

/*-----------------------------------------------------------------------*/
LSTM_Harmonizer* lstm_harmonizer_new(char* folder)
{
  int folder_len = strlen(folder);
  int path_len = folder_len + 50;
  char buffer[path_len];

  LSTM_Harmonizer* self = calloc(1, sizeof(*self));

  if(self != NULL)
    {
      //leanred parameters
      self->layer_1_weights     = matrix_new_from_numpy_file(lstm_harmonizer_filename(folder, "layer_1_weights.npy", buffer));
      self->layer_1_biases      = matrix_new_from_numpy_file(lstm_harmonizer_filename(folder, "layer_1_biases.npy" , buffer));
      self->lstm_Wf             = matrix_new_from_numpy_file(lstm_harmonizer_filename(folder, "Wf.npy", buffer));
      self->lstm_Wi             = matrix_new_from_numpy_file(lstm_harmonizer_filename(folder, "Wi.npy", buffer));
      self->lstm_Wo             = matrix_new_from_numpy_file(lstm_harmonizer_filename(folder, "Wo.npy", buffer));
      self->lstm_Wg             = matrix_new_from_numpy_file(lstm_harmonizer_filename(folder, "Wg.npy", buffer));
      self->lstm_Uf             = matrix_new_from_numpy_file(lstm_harmonizer_filename(folder, "Uf.npy", buffer));
      self->lstm_Ui             = matrix_new_from_numpy_file(lstm_harmonizer_filename(folder, "Ui.npy", buffer));
      self->lstm_Uo             = matrix_new_from_numpy_file(lstm_harmonizer_filename(folder, "Uo.npy", buffer));
      self->lstm_Ug             = matrix_new_from_numpy_file(lstm_harmonizer_filename(folder, "Ug.npy", buffer));
      self->lstm_bf             = matrix_new_from_numpy_file(lstm_harmonizer_filename(folder, "bf.npy", buffer));
      self->lstm_bi             = matrix_new_from_numpy_file(lstm_harmonizer_filename(folder, "bi.npy", buffer));
      self->lstm_bo             = matrix_new_from_numpy_file(lstm_harmonizer_filename(folder, "bo.npy", buffer));
      self->lstm_bg             = matrix_new_from_numpy_file(lstm_harmonizer_filename(folder, "bg.npy", buffer));
      self->layer_3_weights     = matrix_new_from_numpy_file(lstm_harmonizer_filename(folder, "layer_3_weights.npy", buffer));
      self->layer_3_biases      = matrix_new_from_numpy_file(lstm_harmonizer_filename(folder, "layer_3_biases.npy" , buffer));
  
      if((self->layer_1_weights == NULL) || (self->layer_1_biases == NULL) || (self->lstm_Wf         == NULL) ||
         (self->lstm_Wi         == NULL) || (self->lstm_Wo        == NULL) || (self->lstm_Wg         == NULL) ||
         (self->lstm_Uf         == NULL) || (self->lstm_Ui        == NULL) || (self->lstm_Uo         == NULL) ||
         (self->lstm_Ug         == NULL) || (self->lstm_bf        == NULL) || (self->lstm_bi         == NULL) ||
         (self->lstm_bo         == NULL) || (self->lstm_bg        == NULL) || (self->layer_3_weights == NULL) ||
         (self->layer_3_biases  == NULL))
         {fprintf(stderr, "error reading .npy matrices\r\n");  return lstm_harmonizer_destroy(self);}

      self->num_layer_1_inputs  = matrix_get_num_cols(self->layer_1_weights);
      self->num_layer_2_inputs  = matrix_get_num_rows(self->layer_1_weights);
      self->num_layer_3_inputs  = matrix_get_num_cols(self->layer_3_weights);
      self->num_outputs         = matrix_get_num_rows(self->layer_3_weights);
      //self->num_audio_inputs    = self->num_layer_1_inputs - self->num_outputs;
      self->num_audio_inputs    = self->num_layer_1_inputs - self->num_outputs - 1;

      self->audio_features      = matrix_new(self->num_audio_inputs  , 1);
      self->input_vector        = matrix_new(self->num_layer_1_inputs, 1);
      self->layer_1_out         = matrix_new(self->num_layer_2_inputs, 1);
      self->lstm_h_t            = matrix_new(self->num_layer_3_inputs, 1);
      self->lstm_c_t            = matrix_new(self->num_layer_3_inputs, 1);
      self->lstm_f_t            = matrix_new(self->num_layer_3_inputs, 1);
      self->lstm_i_t            = matrix_new(self->num_layer_3_inputs, 1);
      self->lstm_o_t            = matrix_new(self->num_layer_3_inputs, 1);
      self->lstm_g_t            = matrix_new(self->num_layer_3_inputs, 1);
      self->lstm_temp_w         = matrix_new(self->num_layer_3_inputs, 1);
      self->lstm_temp_u         = matrix_new(self->num_layer_3_inputs, 1);
      self->layer_3_out         = matrix_new(self->num_outputs,        1);
      if((self->audio_features  == NULL) || (self->input_vector == NULL) || (self->layer_1_out == NULL) ||
         (self->lstm_h_t        == NULL) || (self->lstm_c_t     == NULL) || (self->lstm_f_t    == NULL) ||
         (self->lstm_i_t        == NULL) || (self->lstm_o_t     == NULL) || (self->lstm_g_t    == NULL) ||
         (self->lstm_temp_w     == NULL) || (self->lstm_temp_u  == NULL) || (self->layer_3_out == NULL))
         {fprintf(stderr, "error allocating new matrices\r\n");  return lstm_harmonizer_destroy(self);}

     //check conformability of matrices read from disk
      if(self->num_audio_inputs != 572)
        {fprintf(stderr, "expecting 572 audio inputs for spectral whitening\r\n");  return lstm_harmonizer_destroy(self);}
      if(!matrix_has_shape(self->layer_1_weights, self->num_layer_2_inputs, self->num_layer_1_inputs) ||
         !matrix_has_shape(self->layer_1_biases , self->num_layer_2_inputs, 1)                        ||
         !matrix_has_shape(self->lstm_Wf        , self->num_layer_3_inputs, self->num_layer_2_inputs) ||
         !matrix_has_shape(self->lstm_bf        , self->num_layer_3_inputs, 1)                        ||
         !matrix_has_shape(self->layer_3_weights, self->num_outputs       , self->num_layer_3_inputs) ||
         !matrix_has_shape(self->layer_3_biases , self->num_outputs       , 1)                        ||
         !matrix_has_same_shape_as(self->lstm_Wf, self->lstm_Wi) ||
         !matrix_has_same_shape_as(self->lstm_Wf, self->lstm_Wo) ||
         !matrix_has_same_shape_as(self->lstm_Wf, self->lstm_Wg) ||
         !matrix_has_same_shape_as(self->lstm_Wf, self->lstm_Uf) ||
         !matrix_has_same_shape_as(self->lstm_Wf, self->lstm_Ui) ||
         !matrix_has_same_shape_as(self->lstm_Wf, self->lstm_Uo) ||
         !matrix_has_same_shape_as(self->lstm_Wf, self->lstm_Ug) ||
         !matrix_has_same_shape_as(self->lstm_bf, self->lstm_bi) ||
         !matrix_has_same_shape_as(self->lstm_bf, self->lstm_bo) ||
         !matrix_has_same_shape_as(self->lstm_bf, self->lstm_bg))
         {fprintf(stderr, "conformability errors in saved matrices\r\n");  return lstm_harmonizer_destroy(self);}
      
      //self->autoregressive_histogram = matrix_new(self->num_outputs, 1);
      //self->histogram_coeff = 0.75;
      //if(self->autoregressive_histogram == NULL)
      //  return lstm_harmonizer_destroy(self);


      self->filter = organ_pipe_filter_new(WINDOW_SIZE, ORGAN_PIPE_FILTER_MODE_PADDED_DFT);
      //self->filter = organ_pipe_filter_new(self->num_audio_inputs, ORGAN_PIPE_FILTER_MODE_AUTOCORRELATION);
      if(self->filter == NULL)
        {fprintf(stderr, "unable to create organ pipe filter\r\n");  return lstm_harmonizer_destroy(self);}
        
      self->notes_changed_callback = NULL;
      self->lowest_midi_note = 36;
      
      lstm_harmonizer_init_band_center_freqs(self);
    }
  return self;
}

/*-----------------------------------------------------------------------*/
LSTM_Harmonizer*       lstm_harmonizer_destroy               (LSTM_Harmonizer* self)
{
  if(self != NULL)
    {
      self->filter          = organ_pipe_filter_destroy(self->filter);
      self->audio_features  = matrix_destroy(self->audio_features);
      self->input_vector    = matrix_destroy(self->input_vector);
      self->layer_1_weights = matrix_destroy(self->layer_1_weights);
      self->layer_1_biases  = matrix_destroy(self->layer_1_biases);
      self->layer_1_out     = matrix_destroy(self->layer_1_out);
      self->lstm_Wf         = matrix_destroy(self->lstm_Wf);
      self->lstm_Wi         = matrix_destroy(self->lstm_Wi);
      self->lstm_Wo         = matrix_destroy(self->lstm_Wo);
      self->lstm_Wg         = matrix_destroy(self->lstm_Wg);
      self->lstm_Uf         = matrix_destroy(self->lstm_Uf);
      self->lstm_Ui         = matrix_destroy(self->lstm_Ui);
      self->lstm_Uo         = matrix_destroy(self->lstm_Uo);
      self->lstm_Ug         = matrix_destroy(self->lstm_Ug);
      self->lstm_bf         = matrix_destroy(self->lstm_bf);
      self->lstm_bi         = matrix_destroy(self->lstm_bi);
      self->lstm_bo         = matrix_destroy(self->lstm_bo);
      self->lstm_bg         = matrix_destroy(self->lstm_bg);
      self->lstm_h_t        = matrix_destroy(self->lstm_h_t);
      self->lstm_c_t        = matrix_destroy(self->lstm_c_t);
      self->lstm_f_t        = matrix_destroy(self->lstm_f_t);
      self->lstm_i_t        = matrix_destroy(self->lstm_i_t);
      self->lstm_o_t        = matrix_destroy(self->lstm_o_t);
      self->lstm_g_t        = matrix_destroy(self->lstm_g_t);
      self->lstm_temp_w     = matrix_destroy(self->lstm_temp_w);
      self->lstm_temp_u     = matrix_destroy(self->lstm_temp_u);
      self->layer_3_weights = matrix_destroy(self->layer_3_weights);
      self->layer_3_biases  = matrix_destroy(self->layer_3_biases);
      self->layer_3_out     = matrix_destroy(self->layer_3_out);
      
      free(self);
    }
  return (LSTM_Harmonizer* ) NULL;
}

/*-----------------------------------------------------------------------*/
void lstm_harmonizer_set_notes_changed_callback(LSTM_Harmonizer* self, lstm_harmonizer_notes_changed_callback_t callback,  void* callback_self)
{
  self->notes_changed_callback = callback;
  self->notes_changed_callback_self = callback_self;
}


/*-----------------------------------------------------------------------*/
matrix_val_t lstm_harmonizer_sigmoid(void* SELF, matrix_val_t x)
{
  return (1.0 / (1.0 + exp(-x)));
}

/*-----------------------------------------------------------------------*/
matrix_val_t lstm_harmonizer_tanh(void* SELF, matrix_val_t x)
{
  return tanh(x);
}

/*-----------------------------------------------------------------------*/
matrix_val_t lstm_harmonizer_relu(void* SELF, matrix_val_t x)
{
  return (x<0) ? 0 : x;
}

/*-----------------------------------------------------------------------*/
unsigned lstm_harmonizer_argmax(Matrix* input)
{
  unsigned i, n = matrix_get_num_rows(input);
  matrix_val_t* arr = matrix_get_values_array(input);
	unsigned argmax = 0;
  
	for(i=1; i<n; i++)
		if(arr[i] > arr[argmax])
			argmax = i;
      
  return argmax;
}

/*-----------------------------------------------------------------------*/
/*
void lstm_harmonizer_softmax(Matrix* input, double temperature)
{
  if(temperature < 0.001)
    temperature = 0.001;
    
  unsigned i;
  matrix_multiply_scalar(input, 1.0/temperature, NULL);
  matrix_val_t* arr = matrix_get_values_array(input);
  unsigned n = matrix_get_num_rows(input);
	matrix_val_t max = arr[0];
  matrix_val_t sum = 0;
  matrix_val_t constant;

	for(i=1; i<n; i++)
		if(arr[i] > max)
			max = arr[i];

	for(i=0; i<n; i++)
		sum += exp(arr[i] - max);

	constant = max + log(sum);
	for (i=0; i<n; i++)
		arr[i] = exp(arr[i] - constant);
}
*/

/*-----------------------------------------------------------------------*/
void lstm_harmonizer_init_state(LSTM_Harmonizer* self)
{
  pthread_mutex_lock(&self->clear_mutex);
  matrix_fill_zeros(self->lstm_h_t);
  matrix_fill_zeros(self->lstm_c_t);
  //matrix_fill_zeros(self->autoregressive_histogram);
  pthread_mutex_unlock(&self->clear_mutex);
}

/*-----------------------------------------------------------------------*/
Matrix* lstm_harmonizer_forward(LSTM_Harmonizer* self, Matrix* inputs)
{
  //linear fully connected layer
  matrix_multiply(self->layer_1_weights, inputs, self->layer_1_out);
  matrix_add(self->layer_1_out, self->layer_1_biases, NULL);
  matrix_apply_function(self->layer_1_out, lstm_harmonizer_relu, self, NULL);
  
  //LSTM Layer
  //https://en.wikipedia.org/wiki/Long_short-term_memory
  //https://pytorch.org/docs/stable/generated/torch.nn.LSTM.html
  pthread_mutex_lock(&self->clear_mutex);
  matrix_multiply(self->lstm_Wf, self->layer_1_out, self->lstm_temp_w);
  matrix_multiply(self->lstm_Uf, self->lstm_h_t, self->lstm_temp_u);
  matrix_add(self->lstm_temp_w, self->lstm_temp_u, self->lstm_f_t);
  matrix_add(self->lstm_f_t, self->lstm_bf, NULL);
  matrix_apply_function(self->lstm_f_t, lstm_harmonizer_sigmoid, self, NULL);
  
  matrix_multiply(self->lstm_Wi, self->layer_1_out, self->lstm_temp_w);
  matrix_multiply(self->lstm_Ui, self->lstm_h_t, self->lstm_temp_u);
  matrix_add(self->lstm_temp_w, self->lstm_temp_u, self->lstm_i_t);
  matrix_add(self->lstm_i_t, self->lstm_bi, NULL);
  matrix_apply_function(self->lstm_i_t, lstm_harmonizer_sigmoid, self, NULL);

  matrix_multiply(self->lstm_Wo, self->layer_1_out, self->lstm_temp_w);
  matrix_multiply(self->lstm_Uo, self->lstm_h_t, self->lstm_temp_u);
  matrix_add(self->lstm_temp_w, self->lstm_temp_u, self->lstm_o_t);
  matrix_add(self->lstm_o_t, self->lstm_bo, NULL);
  matrix_apply_function(self->lstm_o_t, lstm_harmonizer_sigmoid, self, NULL);

  matrix_multiply(self->lstm_Wg, self->layer_1_out, self->lstm_temp_w);
  matrix_multiply(self->lstm_Ug, self->lstm_h_t, self->lstm_temp_u);
  matrix_add(self->lstm_temp_w, self->lstm_temp_u, self->lstm_g_t);
  matrix_add(self->lstm_g_t, self->lstm_bg, NULL);
  matrix_apply_function(self->lstm_g_t, lstm_harmonizer_tanh, self, NULL);
  
  matrix_multiply_pointwise(self->lstm_f_t, self->lstm_c_t, NULL);
  matrix_multiply_pointwise(self->lstm_i_t, self->lstm_g_t, NULL);
  matrix_add(self->lstm_f_t, self->lstm_i_t, self->lstm_c_t);
  
  matrix_apply_function(self->lstm_c_t, lstm_harmonizer_tanh, self, self->lstm_h_t);
  matrix_multiply_pointwise(self->lstm_h_t, self->lstm_o_t, NULL);
  
  //Final linear fully connected layer
  matrix_multiply(self->layer_3_weights, self->lstm_h_t, self->layer_3_out);
  pthread_mutex_unlock(&self->clear_mutex);
  
  matrix_add(self->layer_3_out, self->layer_3_biases, NULL);
  
  return self->layer_3_out;
}

/*-----------------------------------------------------------------------*/
void lstm_harmonizer_process_audio(LSTM_Harmonizer* self, auSample_t* buffer, int num_frames)
{
  organ_pipe_filter_process(self->filter, buffer, num_frames, lstm_harmonizer_stft_process_callback, self);
}

/*-----------------------------------------------------------------------*/
void lstm_harmonizer_init_band_center_freqs(LSTM_Harmonizer* self)
{
  int b;
  double c_b;
  //this ends up ranging from 26.0 Hz to 6935 Hz
  for(b=0; b<NUM_BANDS+2; b++)
    {
      c_b = 229 * (pow(10, (b+1) / 21.4) - 1);
      c_b = dft_bin_of_frequency(c_b, 44100, K);

      //todo: could get better low-freq by not rounding and doing proper interpolation
      self->band_center_freq_indices[b] = round(c_b);
      if(self->band_center_freq_indices[b] < 1)
        self->band_center_freq_indices[b] = 1;
      if(self->band_center_freq_indices[b] >= WINDOW_SIZE)
        self->band_center_freq_indices[b] = WINDOW_SIZE-1;
    }
}

/*-----------------------------------------------------------------------*/
//whitens magnitude from indices self->band_center_freq_indices[1] to self->band_center_freq_indices[NUM_BANDS] inclusive
//k=5 to k=576, inclusive
void lstm_harmonizer_spectral_whitening(LSTM_Harmonizer* self, dft_sample_t* magnitude, int N)
{
  int k, b;
  double bands[NUM_BANDS] = {0};
  double step;
  double H_b_k;
  int* centers = self->band_center_freq_indices;
  double nu_minus_1 = 0.33 - 1;
  double gamma_k;
  
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
      bands[b] /= (double)(K);
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
void lstm_harmonizer_stft_process_callback_for_testing(void* SELF, dft_sample_t* real, int N)
{
  LSTM_Harmonizer* self = SELF;
  static int i = 0;
  int notes[10];
  int num_notes = 0;
  //74, 68, 63, 57, 54, 61, 65, 70
  
  if(i==0)
    {
      notes[0] = 74;
      num_notes = 1;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }

  else if(i==44)
    {
      notes[0] = 68;
      num_notes = 1;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==88)
    {
      //notes[0] = 74;
      num_notes = 0;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }

  if(i==132)
    {
      notes[0] = 63;
      num_notes = 1;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==176)
    {
      notes[0] = 57;
      num_notes = 1;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==220)
    {
      notes[0] = 54;
      num_notes = 1;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==264)
    {
      notes[0] = 61;
      num_notes = 1;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==308)
    {
      notes[0] = 65;
      num_notes = 1;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==352)
    {
      notes[0] = 70;
      num_notes = 1;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==396)
    {
      notes[0] = 70;
      notes[1] = 74;
      num_notes = 2;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==440)
    {
      notes[0] = 65;
      notes[1] = 74;
      num_notes = 2;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==484)
    {
      notes[0] = 65;
      notes[1] = 68;
      num_notes = 2;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==528)
    {
      notes[0] = 61;
      notes[1] = 68;
      num_notes = 2;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==572)
    {
      notes[0] = 61;
      notes[1] = 63;
      num_notes = 2;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==616)
    {
      notes[0] = 54;
      notes[1] = 57;
      num_notes = 2;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==660)
    {
      notes[0] = 61;
      notes[1] = 63;
      num_notes = 2;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==704)
    {
      notes[0] = 65;
      notes[1] = 68;
      num_notes = 2;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==748)
    {
      notes[0] = 70;
      notes[1] = 74;
      num_notes = 2;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==792)
    {
      num_notes = 0;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==836)
    {
      notes[0] = 74;
      notes[1] = 70;
      notes[2] = 65;
      num_notes = 3;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==880)
    {
      notes[0] = 61;
      notes[1] = 65;
      notes[2] = 68;
      num_notes = 3;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==924)
    {
      notes[0] = 54;
      notes[1] = 57;
      notes[2] = 61;
      num_notes = 3;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  else if(i==968)
    {
      num_notes = 0;
      self->notes_changed_callback(self->notes_changed_callback_self, notes, num_notes);
    }
  ++i;
}

/*-----------------------------------------------------------------------*/
/*
int lstm_harmonizer_check_notes_changed(LSTM_Harmonizer* self, int curr_notes[MAXNUM_NOTES], unsigned num_curr_notes)
{
  int i, did_change = 0;
  
  if(num_curr_notes != self->num_prev_notes_out)
    did_change = 1;
  
  self->num_prev_notes_out = num_curr_notes;
  for(i-=0; i<num_curr_notes; i++)
    {
      if(curr_notes[i] != self->prev_notes_out[i])
        did_change = 1;
      self->prev_notes_out[i] = curr_notes[i];
    }
  return did_change;
}
*/
/*-----------------------------------------------------------------------*/
/*
void lstm_harmonizer_poly_pitch_tracker_stft_process_callback(void* SELF, dft_sample_t* magnitude, int N)
{
  LSTM_Harmonizer* self = SELF;

  int      i;
  int      curr_notes[MAXNUM_NOTES] = {0};
  unsigned num_curr_notes = 0;

  for(i=0; i<N; i++)
    matrix_set_value(self->input_vector, i, 0, magnitude[i]);

  matrix_copy_partial(self->layer_3_out, self->input_vector, 0, 0, self->num_audio_inputs, 0, self->num_outputs, 1);
  
  Matrix* outputs = lstm_harmonizer_forward(self, self->input_vector);
  
  //todo: when training model we do not need to explicitly signal silence, then get rid of this line
  matrix_set_value(outputs, self->num_outputs-1, 0, 0);
  
  for(i=0; i<self->num_outputs; i++)
    {
      if(matrix_get_value(outputs, i, 0, NULL) > 0.1)
        {
          if(num_curr_notes < (MAXNUM_NOTES-1))
            curr_notes[num_curr_notes++] = i + self->lowest_midi_note;
          matrix_set_value(outputs, i, 0, 1);
        }
      else
        matrix_set_value(outputs, i, 0, 0);
    }
  
  if(lstm_harmonizer_check_notes_changed(self, curr_notes, num_curr_notes))
    {
      if(self->notes_changed_callback != NULL)
        self->notes_changed_callback(self->notes_changed_callback_self, curr_notes, num_curr_notes);
      
      fprintf(stderr, "MIDI: ");
      for(i=0; i<num_curr_notes; i++)
        fprintf(stderr, "%i  ", curr_notes[i]);
      fprintf(stderr, "\r\n");
    }
}
*/

/*-----------------------------------------------------------------------*/
void lstm_harmonizer_stft_process_callback(void* SELF, dft_sample_t* magnitude, int N)
{
  LSTM_Harmonizer* self = SELF;
  
  int i;
  
  lstm_harmonizer_spectral_whitening(self, magnitude, N);

  for(i=5; i<=576; i++)
    matrix_set_value(self->input_vector, i-5, 0, magnitude[i]);

  //matrix_copy_partial(self->autoregressive_histogram, self->input_vector, 0, 0, self->num_audio_inputs, 0, self->num_outputs, 1);
  matrix_copy_partial(self->layer_3_out, self->input_vector, 0, 0, self->num_audio_inputs, 0, self->num_outputs, 1);
  matrix_set_value(self->input_vector, self->num_audio_inputs+self->num_outputs, 0, self->note_timer);
  
  Matrix* outputs = lstm_harmonizer_forward(self, self->input_vector);
  //lstm_harmonizer_softmax_with_temperature(outputs, 0.01);
  
  int  chosen_output_index = lstm_harmonizer_argmax(outputs);
  int note = 0;
  if(chosen_output_index < matrix_get_num_rows(outputs)-1)
    note = self->lowest_midi_note + chosen_output_index;

  //if(note == self->prev_note_out)
    //++self->note_timer;
  //else
    //self->note_timer = 1;
  
  if(self->prev_note_out != note)
    if(self->notes_changed_callback != NULL)
      {
        self->notes_changed_callback(self->notes_changed_callback_self, &note, (note==0) ? 0 : 1);
        fprintf(stderr, "MIDI: %i\r\n", note);
      }
  
  //make output onehot for next input
  matrix_fill_zeros(outputs);
  matrix_set_value(outputs, chosen_output_index, 0, 1);
  self->prev_note_out = note;
  
  //matrix_multiply_scalar(self->autoregressive_histogram, self->histogram_coeff, NULL);
  //matrix_set_value      (self->autoregressive_histogram, chosen_output_index, 0, 1.0);
  //self->prev_note_out = note;
}

/*-----------------------------------------------------------------------*/
Organ_Pipe_Filter* lstm_harmonizer_get_organ_pipe_filter(LSTM_Harmonizer* self)
{
  return self->filter;
}

/*-----------------------------------------------------------------------*/
#include "Timestamp.h"
void lstm_harmonizer_test_io(LSTM_Harmonizer* self, matrix_val_t* input_arr, matrix_val_t* output_arr)
{
  Matrix* target_output = matrix_new_copy(self->layer_3_out);
  matrix_set_values_array(target_output, output_arr);
  
  matrix_set_values_array(self->input_vector, input_arr);


  /*
  Matrix* actual_out;
  unsigned i, num_tests = 10000;

  timestamp_microsecs_t start_time = timestamp_get_current_time();
  for(i=0; i<num_tests; i++)
    actual_out = lstm_harmonizer_forward(self, self->input_vector);
  timestamp_microsecs_t end_time = timestamp_get_current_time();
  
  fprintf(stderr, "%i timesteps in %f secs\r\n", num_tests, (end_time-start_time) / 1000000.0);
  */

  //lstm_harmonizer_forward(self, self->input_vector);
  lstm_harmonizer_forward(self, self->input_vector);
  lstm_harmonizer_forward(self, self->input_vector);
  lstm_harmonizer_forward(self, self->input_vector);
  Matrix* actual_out = lstm_harmonizer_forward(self, self->input_vector);

  fprintf(stderr, "TARGET OUTPUT:\r\n");
  matrix_print(target_output);
  
  fprintf(stderr, "ACTUAL OUTPUT:\r\n");
  matrix_print(actual_out);

  matrix_destroy(target_output);
}
