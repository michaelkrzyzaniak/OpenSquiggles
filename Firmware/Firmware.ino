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
  //interface_run_loop();
}
