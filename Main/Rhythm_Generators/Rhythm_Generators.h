/*
 *  Synth.h
 *  Make weird noises
 *
 *  Made by Michael Krzyzaniak at Arizona State University's
 *  School of Arts, Media + Engineering in Spring of 2013
 *  mkrzyzan@asu.edu
 */

#ifndef __RHYTHM_GENERATOR__
#define __RHYTHM_GENERATOR__ 1

#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

#include <stdio.h> //testing only!

//Protected Stuff
#include "../../Beat-and-Tempo-Tracking/BTT.h"

#define RHYTHM_GENERATOR_SUBCLASS_GUTS                                 \
  void*       (*destroy) (void*);                                      \
  const char* (*name)    (void*);                                      \
  void        (*onset)   (void* SELF, BTT*, unsigned long long);             \
  int         (*beat)    (void* SELF, BTT*, unsigned long long, float*, int);\

//Private Stuff
typedef struct rhythm_dummy_cast_struct
{
  RHYTHM_GENERATOR_SUBCLASS_GUTS;
}Rhythm;

//Public Stuff. If you make a new module, add its
//constructor here and to the constructor array below.
typedef Rhythm* (*rhythm_new_funct)(BTT*);

Rhythm* rhythm_template_new(BTT* btt);
Rhythm* rhythm_random_beat_from_list_new(BTT* btt);
Rhythm* rhythm_two_beat_delay_new(BTT* btt);
//Rhythm* rhythm_random_beat_new(BTT* btt);

static const rhythm_new_funct rhythm_constructors[] =
{
  rhythm_random_beat_from_list_new,
  rhythm_two_beat_delay_new,
  rhythm_template_new,
  //rhythm_random_beat_new,
};

static const int   rhythm_num_constructors = sizeof(rhythm_constructors) / sizeof(*rhythm_constructors);

Rhythm*      rhythm_destroy (Rhythm* self);
const char*  rhythm_get_name(Rhythm* self);
void         rhythm_onset   (Rhythm* self, BTT* beat_tracker, unsigned long long sample_time);
int          rhythm_beat    (Rhythm* self, BTT* beat_tracker, unsigned long long sample_time, float* returned_rhythm, int returned_rhythm_maxlen);

//dont worry about this, nothing to see here
#define rhythm_destroy(s)          ((Rhythm*)(s))->destroy ((s));
#define rhythm_get_name(s)         ((Rhythm*)(s))->name    ((s));
#define rhythm_onset(s, a, b)      ((Rhythm*)(s))->onset   ((s), (a), (b));
#define rhythm_beat(s, a, b, c, d) ((Rhythm*)(s))->beat    ((s), (a), (b), (c), (d));


#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif   // __RHYTHM_GENERATOR__
