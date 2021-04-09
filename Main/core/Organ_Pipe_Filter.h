/*-------------------------------------------------------------------------

-------------------------------------------------------------------------*/

#ifndef __ORGAN_PIPE_FILTER__
#define __ORGAN_PIPE_FILTER__ 1

#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

#include "../../Beat-and-Tempo-Tracking/src/DFT.h"
#include "constants.h"


/*--------------------------------------------------------------------*/
typedef struct Opaque_Organ_Pipe_Filter_Struct Organ_Pipe_Filter;
typedef void (*organ_pipe_filter_onprocess_t)(void* onprocess_self, dft_sample_t* real, int N);

Organ_Pipe_Filter* organ_pipe_filter_new(int window_size /*power of 2 please*/);
Organ_Pipe_Filter* organ_pipe_filter_destroy(Organ_Pipe_Filter* self);
void organ_pipe_filter_notify_sounding_notes(Organ_Pipe_Filter* self, int sounding_notes[OP_NUM_SOLENOIDS]);
void organ_pipe_filter_process(Organ_Pipe_Filter* self, dft_sample_t* real_input, int len, organ_pipe_filter_onprocess_t onprocess, void* onprocess_self);

#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif   // __ORGAN_PIPE_FILTER__
