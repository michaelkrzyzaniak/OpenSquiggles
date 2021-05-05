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

#define POLY_HARMONIZER_DEFAULT_ON_FOR  2
#define POLY_HARMONIZER_DEFAULT_OFF_FOR 2
#define POLY_HARMONIZER_DEFAULT_MIN_NOTE  36
#define POLY_HARMONIZER_DEFAULT_MAX_NOTE 96
#define POLY_HARMONIZER_DEFAULT_RESOLUTION  3.0
#define POLY_HARMONIZER_DEFAULT_MAX_POLYPHONY 6
#define POLY_HARMONIZER_DEFAULT_DELTA  0
#define POLY_HARMONIZER_DEFAULT_M 20

typedef void (*poly_harmonizer_notes_changed_callback_t)(void* SELF, int* midi_notes, int num_notes);

typedef struct opaque_poly_harmonizer_struct Poly_Harmonizer;

Poly_Harmonizer* poly_harmonizer_new                      (double sample_rate);
Poly_Harmonizer* poly_harmonizer_destroy                  (Poly_Harmonizer* self);
void poly_harmonizer_set_notes_changed_callback(Poly_Harmonizer* self, poly_harmonizer_notes_changed_callback_t callback,  void* callback_self);
void poly_harmonizer_process_audio(Poly_Harmonizer* self, auSample_t* buffer, int num_frames);
void poly_harmonizer_init_state(Poly_Harmonizer* self);

Organ_Pipe_Filter* poly_harmonizer_get_organ_pipe_filter(Poly_Harmonizer* self);

void    poly_harmonizer_set_on_for (Poly_Harmonizer* self, int on_for);
int     poly_harmonizer_get_on_for (Poly_Harmonizer* self);

void    poly_harmonizer_set_off_for(Poly_Harmonizer* self, int off_for);
int     poly_harmonizer_get_off_for(Poly_Harmonizer* self);

void    poly_harmonizer_set_min_note(Poly_Harmonizer* self, double min_note);
double  poly_harmonizer_get_min_note(Poly_Harmonizer* self);

void    poly_harmonizer_set_max_note(Poly_Harmonizer* self, double max_note);
double  poly_harmonizer_get_max_note(Poly_Harmonizer* self);

void    poly_harmonizer_set_resolution(Poly_Harmonizer* self, double divisions_per_half_step);
double  poly_harmonizer_get_resolution(Poly_Harmonizer* self);

/* this defines the maximum number of notes that can be sounding at once */
void    poly_harmonizer_set_max_polyphony(Poly_Harmonizer* self, int polyphony);
int     poly_harmonizer_get_max_polyphony(Poly_Harmonizer* self);

/* this defines how much each candidate is subtracted out of the spectrum */
void    poly_harmonizer_set_delta(Poly_Harmonizer* self, double delta);
double  poly_harmonizer_get_delta(Poly_Harmonizer* self);

/* this defines how many partials to search */
void    poly_harmonizer_set_M(Poly_Harmonizer* self, int M);
int     poly_harmonizer_get_M(Poly_Harmonizer* self);

#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif   //__POLY_HARMONIZER__
