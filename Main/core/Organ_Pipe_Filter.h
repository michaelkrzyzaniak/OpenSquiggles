/*-------------------------------------------------------------------------

-------------------------------------------------------------------------*/

#ifndef __ORGAN_PIPE_FILTER__
#define __ORGAN_PIPE_FILTER__ 1

#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

#include "../../Beat-and-Tempo-Tracking/src/DFT.h"
#include "constants.h"

#define ORGAN_PIPE_FILTER_DEFAULT_REDUCTION_COEFFICIENT    0.5
#define ORGAN_PIPE_FILTER_DEFAULT_GATE_COEFFICIENT         0.1
#define ORGAN_PIPE_FILTER_DEFAULT_NOISE_CANCEL_COEFFICIENT 0.0

/*--------------------------------------------------------------------*/
typedef enum organ_pipe_filter_mode_enum
{
  /* DFT size window_size */
  ORGAN_PIPE_FILTER_MODE_DFT,
  ORGAN_PIPE_FILTER_MODE_RESYNTHESIZED_AUDIO,
  /* DFT size 2*window_size */
  ORGAN_PIPE_FILTER_MODE_PADDED_DFT,
  ORGAN_PIPE_FILTER_MODE_AUTOCORRELATION,
}organ_pipe_filter_mode_t;


typedef struct Opaque_Organ_Pipe_Filter_Struct Organ_Pipe_Filter;
typedef void (*organ_pipe_filter_onprocess_t)(void* onprocess_self, dft_sample_t* real, int N);

Organ_Pipe_Filter* organ_pipe_filter_new(int window_size /*power of 2 please*/, organ_pipe_filter_mode_t mode);
Organ_Pipe_Filter* organ_pipe_filter_destroy(Organ_Pipe_Filter* self);
void organ_pipe_filter_notify_sounding_notes(Organ_Pipe_Filter* self, int sounding_notes[OP_NUM_SOLENOIDS]);
void organ_pipe_filter_process(Organ_Pipe_Filter* self, dft_sample_t* real_input, int len, organ_pipe_filter_onprocess_t onprocess, void* onprocess_self);

/* 0~1, the organ pipe spectra will be multiplied by this before being subtracted out of the audio */
void   organ_pipe_filter_set_reduction_coefficient(Organ_Pipe_Filter* self, double coeff);
double organ_pipe_filter_get_reduction_coefficient(Organ_Pipe_Filter* self);

/* >= 0 this is multiplied by the ambient noise level to get the actual gate threshold */
void   organ_pipe_filter_set_gate_coefficient(Organ_Pipe_Filter* self, double coeff);
double organ_pipe_filter_get_gate_coefficient(Organ_Pipe_Filter* self);

/* >= 0 this is multiplied by the average noise level; any fft bin below this value after filtering will be set to 0 */
void   organ_pipe_filter_set_noise_cancel_coefficient(Organ_Pipe_Filter* self, double coeff);
double organ_pipe_filter_get_noise_cancel_coefficient(Organ_Pipe_Filter* self);

#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif   // __ORGAN_PIPE_FILTER__
