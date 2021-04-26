/*------------------------------------------------------------------------
 _  _                         _             _
| || |__ _ _ _ _ __  ___ _ _ (_)______ _ _ | |_
| __ / _` | '_| '  \/ _ \ ' \| |_ / -_) '_|| ' \
|_||_\__,_|_| |_|_|_\___/_||_|_/__\___|_|(_)_||_|
------------------------------------------------------------------------
Written by Michael Krzyzaniak

Version 2.0 March 21 2021
multi-thread support and many small changes
----------------------------------------------------------------------*/
#ifndef __LSTM_HARMONIZER__
#define __LSTM_HARMONIZER__
  
#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

#include "Matrix.h"
#include "AudioSuperclass.h"
#include "Organ_Pipe_Filter.h"

#include <stdio.h>

typedef void (*lstm_harmonizer_notes_changed_callback_t)(void* SELF, int* midi_notes, int num_notes);

typedef struct opaque_lstm_harmonizer_struct LSTM_Harmonizer;

LSTM_Harmonizer* lstm_harmonizer_new                      (char* folder);
LSTM_Harmonizer* lstm_harmonizer_destroy                  (LSTM_Harmonizer* self);
void        lstm_harmonizer_set_notes_changed_callback(LSTM_Harmonizer* self, lstm_harmonizer_notes_changed_callback_t callback,  void* callback_self);
void lstm_harmonizer_process_audio(LSTM_Harmonizer* self, auSample_t* buffer, int num_frames);
Organ_Pipe_Filter* lstm_harmonizer_get_organ_pipe_filter(LSTM_Harmonizer* self);
void lstm_harmonizer_init_state(LSTM_Harmonizer* self);

void lstm_harmonizer_test_io(LSTM_Harmonizer* self, matrix_val_t* input, matrix_val_t* output);

#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif   //__MATRIX__
