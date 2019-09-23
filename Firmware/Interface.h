#ifndef INTERFACE
#define INTERFACE 1

#include "HardwareSerial.h"
#include "MIDI_Parser.h"

#ifdef __cplusplus
extern "C" {
#endif

void interface_init();
void interface_run_loop();


#ifdef __cplusplus
}
#endif

#endif //INTERFACE
