/*------------------------------------------------------------------------
 _  _                         _             _
| || |__ _ _ _ _ __  ___ _ _ (_)______ _ _ | |_
| __ / _` | '_| '  \/ _ \ ' \| |_ / -_) '_|| ' \
|_||_\__,_|_| |_|_|_\___/_||_|_/__\___|_|(_)_||_|
------------------------------------------------------------------------
Written by Michael Krzyzaniak

----------------------------------------------------------------------*/
#ifndef __POLY_HARMONIZER__
#define __POLY_HARMONIZER__
  
#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

#include "AudioSuperclass.h"
#include "Organ_Pipe_Filter.h"

typedef void (*poly_harmonizer_notes_changed_callback_t)(void* SELF, int* midi_notes, int num_notes);

typedef struct opaque_poly_harmonizer_struct Poly_Harmonizer;

Poly_Harmonizer* poly_harmonizer_new                      (double sample_rate);
Poly_Harmonizer* poly_harmonizer_destroy                  (Poly_Harmonizer* self);
void poly_harmonizer_set_notes_changed_callback(Poly_Harmonizer* self, poly_harmonizer_notes_changed_callback_t callback,  void* callback_self);
void poly_harmonizer_process_audio(Poly_Harmonizer* self, auSample_t* buffer, int num_frames);
Organ_Pipe_Filter* poly_harmonizer_get_organ_pipe_filter(Poly_Harmonizer* self);
void poly_harmonizer_init_state(Poly_Harmonizer* self);


#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif   //__POLY_HARMONIZER__
