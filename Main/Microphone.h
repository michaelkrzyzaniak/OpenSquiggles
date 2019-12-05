/*
 *  Synth.h
 *  Make weird noises
 *
 *  Made by Michael Krzyzaniak at Arizona State University's
 *  School of Arts, Media + Engineering in Spring of 2013
 *  mkrzyzan@asu.edu
 */

#ifndef __MICROPHONE__
#define __MICROPHONE__ 1

#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

#include "AudioSuperclass.h"
#include "Rhythm_Generators/Rhythm_Generators.h"
#include "../Beat-and-Tempo-Tracking/BTT.h"

typedef struct OpaqueMicrophoneStruct Microphone;

Microphone*       mic_new                        ();
BTT*              mic_get_btt                    (Microphone* self);
Rhythm*           mic_set_rhythm_generator       (Microphone* self, rhythm_new_funct constructor);
Rhythm*           mic_set_rhythm_generator_index (Microphone* self, int index);
int               mic_get_rhythm_generator_index (Microphone* self);
Rhythm*           mic_get_rhythm_generator       (Microphone* self);

void              mic_set_should_play_beat_bell  (Microphone* self, int should);
int               mic_get_should_play_beat_bell  (Microphone* self);
void              mic_set_quantization_order     (Microphone* self, int order);
int               mic_get_quantization_order     (Microphone* self);

int               mic_audio_callback             (void* SELF, auSample_t* buffer, int num_frames, int num_channels);

//Microphone*  mic_destroy             (Microphone*      self      );
//call with self->destroy(self);

#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif   // __MICROPHONE__
