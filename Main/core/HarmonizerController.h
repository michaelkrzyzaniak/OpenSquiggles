#ifndef __HARMONIZER_CONTROLLER__
#define __HARMONIZER_CONTROLLER__ 1

#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

#include "AudioSuperclass.h"
#include "../../Robot_Communication_Framework/Robot_Communication.h"

typedef struct OpaqueHarmonizerControllerStruct HarmonizerController;

HarmonizerController*       harmonizer_controller_new                        ();
int                         harmonizer_controller_audio_callback             (void* SELF, auSample_t* buffer, int num_frames, int num_channels);

//Microphone*  mic_destroy             (Microphone*      self      );
//call with self->destroy(self);

#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif   // __HARMONIZER_CONTROLLER__
