#include "Solenoid.h"
#include "Arduino.h"
#include "Interface.h"

/*----------------------------------------------------*/
#define       SOLENOID_TIMER_THREAD_INTERVAL 1000 //usec
#define       SOLENOID_NUM_SOLENOIDS 8
#define       SOLENOID_TIMER_DURATION_SECONDS 0.025
IntervalTimer solenoid_timer_thread;
int           solenoid_duration = (int)((SOLENOID_TIMER_DURATION_SECONDS * 1000000.0 / ((double)SOLENOID_TIMER_THREAD_INTERVAL)) + 0.5);
int           solenoid_current_solenoid = 0;
volatile int  solenoid_pins[SOLENOID_NUM_SOLENOIDS]    = {10, 9, 20, 21, 6, 5, 4, 3};
volatile int  solenoid_timers[SOLENOID_NUM_SOLENOIDS]  = {0};

void solenoid_timer_thread_run_loop(void);

/*----------------------------------------------------*/
void solenoid_init()
{
  int i;
  for(i=0; i<SOLENOID_NUM_SOLENOIDS; i++)
    {
      pinMode(solenoid_pins[i], OUTPUT);
      analogWriteFrequency(solenoid_pins[i], 10000);
    }

  analogWriteResolution(8);

  solenoid_timer_thread.priority(0);
  solenoid_timer_thread.begin(solenoid_timer_thread_run_loop, SOLENOID_TIMER_THREAD_INTERVAL);
}

/*----------------------------------------------------*/
void solenoid_tap (float strength)
{
  /* cycle between indices but exclude index 0 because it is the bell */
  //solenoid_tap_specific(solenoid_current_solenoid+1, strength);
  //++solenoid_current_solenoid; solenoid_current_solenoid %= (SOLENOID_NUM_SOLENOIDS-1);

  float decrement = 1.0/3.0;

  while(strength > 0)
    {

      solenoid_tap_specific(solenoid_current_solenoid+1, 3.0 * min(decrement, strength));
      strength -= decrement;
      ++solenoid_current_solenoid; solenoid_current_solenoid %= (SOLENOID_NUM_SOLENOIDS-1);
    }
  
}

/*----------------------------------------------------*/
void solenoid_ding(float strength)
{
  solenoid_tap_specific(0, strength);
}

/*----------------------------------------------------*/
void solenoid_tap_specific(int index, float strength)
{
  if(strength > 1) strength = 1;
  if(strength < 0) strength = 0;
  if(index >= SOLENOID_NUM_SOLENOIDS) index = SOLENOID_NUM_SOLENOIDS-1;
  if(index < 0) index = 0;
   
  noInterrupts();
  solenoid_timers[index] = solenoid_duration;
  //analogWrite(solenoid_pins[index], strength*255);
  //analogWrite(solenoid_pins[index], 127 + strength*128);
  analogWrite(solenoid_pins[index], 205 + strength*50);
  interrupts(); 
}

/*----------------------------------------------------*/
void solenoid_timer_thread_run_loop(void)
{
  interface_run_loop();
  
  int i;
  for(i=0; i<SOLENOID_NUM_SOLENOIDS; i++)
    {
      if(solenoid_timers[i] > 0)
        {
          --solenoid_timers[i];
          if(solenoid_timers[i] <= 0)
            analogWrite(solenoid_pins[i], 0);
        }
    }
}
