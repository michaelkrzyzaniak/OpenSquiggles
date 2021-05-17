#ifndef __HARMONIZER_CONTROLLER__
#define __HARMONIZER_CONTROLLER__ 1

#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

#include "AudioSuperclass.h"
#include "Poly_Harmonizer.h"
#include "Mono_Harmonizer.h"
#include "../../Robot_Communication_Framework/Robot_Communication.h"

typedef struct OpaqueHarmonizerControllerStruct HarmonizerController;

HarmonizerController* harmonizer_controller_new             ();
void                  harmonizer_controller_clear           (HarmonizerController*  self);
Poly_Harmonizer*      harmonizer_controller_get_harmonizer_1(HarmonizerController*  self);
Mono_Harmonizer*      harmonizer_controller_get_harmonizer_2(HarmonizerController*  self);
void                  harmonizer_controller_set_harmonizer  (HarmonizerController*  self, int harmonizer);
int                   harmonizer_controller_get_harmonizer  (HarmonizerController*  self, int harmonizer);



//Microphone*  mic_destroy             (Microphone*      self      );
//call with self->destroy(self);

#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif   // __HARMONIZER_CONTROLLER__
