#ifndef __BEEP__
#define __BEEP__ 1

#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

#include "AudioSuperclass.h"

typedef struct OpaqueBeepStruct Beep;

Beep*        beep_new               ();
void         beep_set_notes         (Beep* self, int* midi_note_number, int num_notes);

//Beep*  beep_destroy             (Beep*      self      );
//call with self->destroy(self);

#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif   // __BEEP__
