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
#ifndef __MONO_HARMONIZER__
#define __MONO_HARMONIZER__
  
#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

#include "Matrix.h"
#include "AudioSuperclass.h"
#include "Organ_Pipe_Filter.h"

#include <stdio.h>

typedef void (*mono_harmonizer_notes_changed_callback_t)(void* SELF, int* midi_notes, int num_notes);

typedef struct opaque_mono_harmonizer_struct Mono_Harmonizer;

Mono_Harmonizer* mono_harmonizer_new                      ();
Mono_Harmonizer* mono_harmonizer_destroy                  (Mono_Harmonizer* self);
void mono_harmonizer_set_notes_changed_callback(Mono_Harmonizer* self, mono_harmonizer_notes_changed_callback_t callback,  void* callback_self);
void mono_harmonizer_process_audio(Mono_Harmonizer* self, auSample_t* buffer, int num_frames);
Organ_Pipe_Filter* mono_harmonizer_get_organ_pipe_filter(Mono_Harmonizer* self);
void mono_harmonizer_init_state(Mono_Harmonizer* self);


#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif   //__MATRIX__
