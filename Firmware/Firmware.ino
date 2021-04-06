/*
Compile this using the Arduino IDE
Board: Teensy 3.2
USB Type: MIDI
*/

#include "Interface.h"
#include "Solenoid.h"
#include "Eye.h"

/*----------------------------------------------*/
void setup(void)
{
  eye_init_module();
  solenoid_init();
  interface_init();
}

/*----------------------------------------------*/
void loop(void) 
{
  //interface is updated on solenoid loop
  //which has the best priority and runs every 1 ms
  //interface_run_loop();
}
