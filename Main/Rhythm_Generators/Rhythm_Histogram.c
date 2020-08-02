/*
 *  Rhythm_histogram.c
 *  To use:
 *    if you want to make a module named "foo",
 *    search and replace "Histogram" with Foo (case sensitive)
 *    and "histogram" with "foo" (case sensitive).
 *    Then declare any variables you need in opaque_rhythm_foo_struct,
 *    initalize them in rhythm_foo_new,
 *    free them if necessary in rhythm_foo_destroy.
 *    Musical onsets will be reported in rhythm_histogram_onset
 *      (this is called automatically by the onset tracker)
 *    You should generate one beat of rhythm at a time, in rhythm_histogram_beat
 *      (this is called automatically by the beat tracker at the begining of each beat)
 *  mkrzyzan@asu.edu
 */

#include "Rhythm_Generators.h"
#include <pthread.h>
#include <math.h>

#include "../extras/Network.h" //calloc
#include "../extras/OSC.h" //calloc

/*--------------------------------------------------------------------*/
void*        rhythm_histogram_destroy (void*);
const char*  rhythm_histogram_name    (void*);
void         rhythm_histogram_onset   (void*, BTT*, unsigned long long);
int          rhythm_histogram_beat    (void*, BTT*, unsigned long long, rhythm_onset_t*, int);

void*        rhythm_histogram_recv_thread_run_loop(void* SELF);

#define NUM_ONSET_TIMES           128
#define MAX_SUBDIVISIONS_PER_BEAT 8
#define MAX_NUM_BEATS             8
#define DEFAULT_SUBDIVISIONS_PER_BEAT 4
#define DEFAULT_NUM_BEATS             4
#define HISTOGRAM_MAX_LENGTH (MAX_SUBDIVISIONS_PER_BEAT * MAX_NUM_BEATS)
#define DEFAULT_DECAY_COEFFICIENT 0.75

#define DEFAULT_OSC_SEND_PORT     9000
#define DEFAULT_OSC_RECV_PORT     9001
#define  OSC_BUFFER_SIZE          512
#define  OSC_VALUES_BUFFER_SIZE   64

/*--------------------------------------------------------------------*/
typedef struct opaque_rhythm_histogram_struct
{
  RHYTHM_GENERATOR_SUBCLASS_GUTS ;
  
  /* add instance variables below here */
  unsigned long long* onset_times;
  unsigned long long  prev_beat_time;
  int                 onsets_index;
  int                 onsets_head_index;
  int                 num_onsets;
  int                 histogram_index;
  
  double*             histogram;
  double              decay_coefficient;
  
  int                 num_beats;
  int                 subdivisions_per_beat;
  
  int                 is_inverse;
  double              nonlinear_exponent;
  
  int                 robot_osc_id;
  unsigned short      osc_send_port;
  Network*            net;
  pthread_t           osc_recv_thread;
  //pthread_mutex_t   onset_buffer_mutex;
  //char* osc_send_buffer;
  char*               osc_recv_buffer;
  oscValue_t*         osc_values_buffer;
  
  int                 experiment_is_running;
  int                 experiment_beat_count;
  
}Rhythm_Histogram;

void         rhythm_histogram_init(Rhythm_Histogram* self);

/*--------------------------------------------------------------------*/
Rhythm* rhythm_histogram_new(BTT* beat_tracker)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*) calloc(1, sizeof(*self));
  
  if(self != NULL)
    {
      self->destroy = rhythm_histogram_destroy;
      self->name    = rhythm_histogram_name;
      self->onset   = rhythm_histogram_onset;
      self->beat    = rhythm_histogram_beat;
    
      /* initalize instance variables below here */
      self->onset_times = calloc(NUM_ONSET_TIMES, sizeof(*self->onset_times));
      if(self->onset_times == NULL) return rhythm_histogram_destroy(self);
    
      self->histogram = calloc(HISTOGRAM_MAX_LENGTH, sizeof(*self->histogram));
      if(self->histogram == NULL) return self->destroy(self);
 
      self->osc_recv_buffer = calloc(1, OSC_BUFFER_SIZE);
      if(!self->osc_recv_buffer) return self->destroy(self);
      self->osc_values_buffer = calloc(sizeof(*self->osc_values_buffer), OSC_VALUES_BUFFER_SIZE);
      if(!self->osc_values_buffer) return self->destroy(self);
      
      self->net = net_new();
      if(!self->net) return rhythm_histogram_destroy(self);
      if(!net_udp_connect(self->net, DEFAULT_OSC_RECV_PORT))
        return self->destroy(self);
      
      self->osc_send_port = DEFAULT_OSC_SEND_PORT;
    
      rhythm_histogram_set_is_inverse (self, 0);
      rhythm_histogram_set_num_beats  (self, DEFAULT_NUM_BEATS);
      rhythm_histogram_set_subdivisions_per_beat(self, DEFAULT_SUBDIVISIONS_PER_BEAT);
      rhythm_histogram_set_nonlinear_exponent(self, 1);
      rhythm_histogram_set_decay_coefficient(self, DEFAULT_DECAY_COEFFICIENT);
      
      rhythm_histogram_init(self);
      
      //if(pthread_mutex_init(&self->onset_buffer_mutex, NULL) != 0)
        //return rhythm_osc_destroy(self);
    
      int error = pthread_create(&self->osc_recv_thread, NULL, rhythm_histogram_recv_thread_run_loop, self);
      if(error != 0)
        return self->destroy(self);
    }
  
  return (Rhythm*)self;
}

/*--------------------------------------------------------------------*/
void rhythm_histogram_init(Rhythm_Histogram* self)
{
  int i;
  //for(i=0; i<HISTOGRAM_MAX_LENGTH; self->histogram[i++] = 0.5);      // all 0.5
  //for(i=0; i<HISTOGRAM_MAX_LENGTH; self->histogram[i++] = random()%2); // 0 or 1 with 50% probability
  for(i=0; i<HISTOGRAM_MAX_LENGTH; self->histogram[i++] = random() / (double)RAND_MAX); //evenly distrobuted on [0 ~ 1)
  self->histogram_index = 0;
  self->onsets_index = 0;
  self->num_onsets = 0;
  self->onsets_head_index = 0;
  
}

/*--------------------------------------------------------------------*/
void*      rhythm_histogram_destroy (void* SELF)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  if(self != NULL)
    {
      /* free any malloc-ed instance variables here */
      if(self->onset_times != NULL)
        free(self->onset_times);
      if(self->histogram != NULL)
        free(self->histogram);
      net_destroy(self->net);
      free(self);
    }
  return (Rhythm*) NULL;
}

/*--------------------------------------------------------------------*/
void  rhythm_histogram_set_is_inverse    (void* SELF, int is_inverse)
{
  /* just return the name of the module for display */
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  self->is_inverse = (is_inverse) ? -1 : 1;
}

/*--------------------------------------------------------------------*/
int  rhythm_histogram_get_is_inverse    (void* SELF)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  return self->is_inverse != 1;
}

/*--------------------------------------------------------------------*/
void  rhythm_histogram_set_num_beats  (void* SELF, int num_beats)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  if(num_beats < 1) num_beats = 1;
  if(num_beats > MAX_NUM_BEATS) num_beats = MAX_NUM_BEATS;
  self->num_beats = num_beats;
}

/*--------------------------------------------------------------------*/
int   rhythm_histogram_get_num_beats  (void* SELF)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  return self->num_beats;
}

/*--------------------------------------------------------------------*/
void  rhythm_histogram_set_subdivisions_per_beat(void* SELF, int subdivisions)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  if(subdivisions < 1) subdivisions = 1;
  if(subdivisions > MAX_SUBDIVISIONS_PER_BEAT) subdivisions = MAX_SUBDIVISIONS_PER_BEAT;
  self->subdivisions_per_beat = subdivisions;
}

/*--------------------------------------------------------------------*/
int   rhythm_histogram_get_subdivisions_per_beat(void* SELF)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  return self->subdivisions_per_beat;
}

/*--------------------------------------------------------------------*/
void   rhythm_histogram_set_nonlinear_exponent(void* SELF, double k)
{
  //x >  0.5: (  1 + abs((2x-1))^k  ) / 2
  //x <= 0.5: (  1 - abs((2x-1))^k  ) / 2
  //switch plus and minus for inverse mode
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  if(k < 0) k = 0;
  self->nonlinear_exponent = k;
}

/*--------------------------------------------------------------------*/
double rhythm_histogram_get_nonlinear_exponent(void* SELF)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  return self->nonlinear_exponent;
}

/*--------------------------------------------------------------------*/
void   rhythm_histogram_set_decay_coefficient(void* SELF, double coeff)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  if(coeff < 0) coeff = 0;
  if(coeff > 1) coeff = 1;
  self->decay_coefficient = coeff;
}

/*--------------------------------------------------------------------*/
double rhythm_histogram_get_decay_coefficient(void* SELF)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  return self->decay_coefficient;
}

/*--------------------------------------------------------------------*/
void   rhythm_histogram_set_robot_osc_id(void* SELF, int id)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  self->robot_osc_id = id;
}

/*--------------------------------------------------------------------*/
int rhythm_histogram_get_robot_osc_id(void* SELF)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  return self->robot_osc_id;
}

/*--------------------------------------------------------------------*/
void   rhythm_histogram_set_osc_send_port(void* SELF, int port)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  self->osc_send_port = port;
}

/*--------------------------------------------------------------------*/
int rhythm_histogram_get_osc_send_port(void* SELF)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  return self->osc_send_port;
}

/*--------------------------------------------------------------------*/
const char*  rhythm_histogram_name    (void* SELF)
{
  /* just return the name of the module for display */
  static const char* name = "Histogram";
  return name;
}

/*--------------------------------------------------------------------*/
void         rhythm_histogram_onset   (void* SELF, BTT* beat_tracker, unsigned long long sample_time)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  //int adjustment = btt_get_beat_prediction_adjustment_audio_samples(beat_tracker);
  //if(sample_time > adjustment) sample_time -= adjustment;// sample_time -= 64;
  self->onset_times[self->onsets_index] = sample_time;
  ++self->onsets_index; self->onsets_index %= NUM_ONSET_TIMES;
  ++self->num_onsets;
  
  /* if buffer is full, shift out the oldest samples */
  if(self->num_onsets > NUM_ONSET_TIMES)
    {
      self->onsets_head_index += (self->num_onsets - NUM_ONSET_TIMES);
      self->onsets_head_index %= NUM_ONSET_TIMES;
      self->num_onsets = NUM_ONSET_TIMES;
    }
}

/*--------------------------------------------------------------------*/
float        rhythm_histogram_get_note_density   (Rhythm_Histogram* self)
{
  float density = 0;
  int i;
  int length = self->num_beats *  self->subdivisions_per_beat;
  for(i=0; i<length; density += self->histogram[i++]);
  density /= (double)length;
  
  return (self->is_inverse == -1) ? 1.0-density : density;
}

/*--------------------------------------------------------------------*/
//try to maximize this value
float        rhythm_histogram_get_convergence_score   (Rhythm_Histogram* self /*, float note_density*/)
{
  float result;
  
  float minimax = 2;
  float maximin = -1;
  int length = self->num_beats * self->subdivisions_per_beat;
  int i;
  for(i=0; i<length; i++)
    {
      if(self->histogram[i] >= 0.5)
        {
          if(self->histogram[i] < minimax)
            minimax = self->histogram[i];
        }
      else
        {
          if(self->histogram[i] > maximin)
            maximin = self->histogram[i];
        }
    }
  
  if((minimax == 2) || (maximin == -1))
    result = 0;
  else
    result = minimax - maximin;
  
  return result;
}

/*--------------------------------------------------------------------*/
void rhythm_histogram_send_osc_convergence_and_density(Rhythm_Histogram* self)
{
  float density     =  rhythm_histogram_get_note_density(self);
  float convergence =  rhythm_histogram_get_convergence_score(self/*, density*/);
  
  fprintf(stderr, "%f\t%f\t", density, convergence);
  
  /*
  int  buffer_n = 36; //I happen to know it will be exactly 36 bytes
  char buffer[buffer_n];
  int  num_bytes = oscConstruct(buffer, buffer_n, "/convergence", "iff", self->robot_osc_id, convergence, density);
  if(num_bytes > 0)
    net_udp_send(self->net, buffer, num_bytes, "255.255.255.255", self->osc_send_port);
  */
}

/*--------------------------------------------------------------------*/
void* rhythm_histogram_recv_thread_run_loop(void* SELF)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  char senders_address[16];
  char *osc_address, *osc_type_tag;
  
  for(;;)
  {
    int num_valid_bytes = net_udp_receive (self->net, self->osc_recv_buffer, OSC_BUFFER_SIZE, senders_address);
    if(num_valid_bytes < 0)
      continue; //return NULL ?
  
    int num_osc_values = oscParse(self->osc_recv_buffer, num_valid_bytes, &osc_address, &osc_type_tag, self->osc_values_buffer, OSC_VALUES_BUFFER_SIZE);
    if(num_osc_values < 0)
        continue;
  
    uint32_t address_hash = oscHash((unsigned char*)osc_address);
    if(address_hash == 5858985) // '/c'
      {
         rhythm_histogram_init(self);
         self->experiment_beat_count = 0;
      }
    else if(address_hash == 5859001) // '/s'
      {
        rhythm_histogram_init(self);
        self->experiment_beat_count = 0;
        self->experiment_is_running = 1;
      }
    else if(address_hash == 5858969) // '/S'
      {
        self->experiment_is_running = 0;
      }
  }
}



#define BEATS_PER_EXPERIMENT 80 //probably make this a multiple of beats per rhythm
//const int RHYTHM_HISTOGRAM_EXPERIMENT_INVERSE_VALUES[] = {0, 1};
//const float RHYTHM_HISTOGRAM_EXPERIMENT_K_VALUES[] = {0, 0.5, 1, 2};
//const float RHYTHM_HISTOGRAM_EXPERIMENT_DECAY_VALUES[] = {0, 0.5, 0.75, 0.825}; //0 is immediate update
//#define NUM_INVERSE_VALUES 2
//#define NUM_K_VALUES 4
//#define NUM_DECAY_VALUES 4

typedef struct
{
  int i;
  float k;
  float d;
}h_params;

#define NUM_TRIALS 5

h_params p[NUM_TRIALS] =
{
  {0, 0, 1},
  {0, 0, 0},
  {0, 0, 0.61803399},
  {1, 0, 0},
  {1, 0, 0.8},
};
/*
h_params p[NUM_TRIALS] =
{
  {0, 0, 1},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
};
*/
/*
h_params p[NUM_TRIALS] =
{
  {0, 0, 1},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
};
*/
/*--------------------------------------------------------------------*/
void rhythm_histogram_update_experiment(Rhythm_Histogram* self)
{
  int beat_number = self->experiment_beat_count % BEATS_PER_EXPERIMENT;
  
  if(beat_number == 0)
    {
      int trial_count = self->experiment_beat_count / BEATS_PER_EXPERIMENT;
      //if(trial_count >= (NUM_INVERSE_VALUES * NUM_K_VALUES * NUM_DECAY_VALUES))
      if(trial_count >= NUM_TRIALS)
        {
          self->experiment_is_running = 0;
          fprintf(stderr, "EXPERIMENT DONE\r\n\r\n\r\n\r\n\r\n");
          return;
        }
      //int decay_index = trial_count % NUM_DECAY_VALUES;
      //int k_index = (trial_count / NUM_DECAY_VALUES) % NUM_K_VALUES;
      //int inverse_index = (trial_count / (NUM_DECAY_VALUES * NUM_K_VALUES)) % NUM_INVERSE_VALUES;
      //rhythm_histogram_set_is_inverse (self, RHYTHM_HISTOGRAM_EXPERIMENT_INVERSE_VALUES[inverse_index]);
      //rhythm_histogram_set_nonlinear_exponent(self, RHYTHM_HISTOGRAM_EXPERIMENT_K_VALUES[k_index]);
      //rhythm_histogram_set_decay_coefficient(self, RHYTHM_HISTOGRAM_EXPERIMENT_DECAY_VALUES[decay_index]);

      rhythm_histogram_set_is_inverse (self, p[trial_count].i);
      rhythm_histogram_set_nonlinear_exponent(self, p[trial_count].k);
      rhythm_histogram_set_decay_coefficient(self, p[trial_count].d);

      fprintf(stderr, "starting trial with d: %f\tk: %f\ti: %i\r\n",
                      p[trial_count].d,
                      p[trial_count].k,
                      p[trial_count].i);
      rhythm_histogram_init(self);
    }
  
  int i;
  for(i=0; i<self->subdivisions_per_beat * self->num_beats; i++)
    fprintf(stderr, "%f\t", self->histogram[i]);
  //fprintf(stderr, "\r\n");
  
  ++self->experiment_beat_count;
}

/*--------------------------------------------------------------------*/
int          rhythm_histogram_beat    (void* SELF, BTT* beat_tracker, unsigned long long sample_time, rhythm_onset_t* returned_rhythm, int returned_rhythm_maxlen)
{
  Rhythm_Histogram* self = (Rhythm_Histogram*)SELF;
  
  int beat_duration = sample_time - self->prev_beat_time;
  if(beat_duration <= 0) return 0;
  
  unsigned long long prev_beat_time = self->prev_beat_time;
  self->prev_beat_time = sample_time;
  
  if(prev_beat_time == 0) return 0;
  
  int i, n=0;
  
  float onset_mask[MAX_SUBDIVISIONS_PER_BEAT] = {0};
  
  //get the onset mask
  while(self->num_onsets > 0)
    {
      float beat_time = (int)(self->onset_times[self->onsets_head_index] - prev_beat_time) / (double)beat_duration;
      beat_time = round(beat_time * self->subdivisions_per_beat);
    
      if(beat_time >= self->subdivisions_per_beat)
        break;
      else if(beat_time >= 0)
        onset_mask[(int)beat_time] = 1;
  
      ++self->onsets_head_index; self->onsets_head_index %= NUM_ONSET_TIMES;
      --self->num_onsets;
    }

  //update and decay the histogram
  for(i=0; i<self->subdivisions_per_beat; i++)
    {
      self->histogram[self->histogram_index + i] *= self->decay_coefficient;
      self->histogram[self->histogram_index + i] += onset_mask[i] * (1.0-self->decay_coefficient);
    }
  
  //if(self->experiment_is_running)
    //rhythm_histogram_send_osc_convergence_and_density(self);
  
  self->histogram_index += self->subdivisions_per_beat;
  self->histogram_index %= self->subdivisions_per_beat * self->num_beats;

  if(self->experiment_is_running)
    {
      rhythm_histogram_update_experiment(self);
      rhythm_histogram_send_osc_convergence_and_density(self);
    }

  //generate a rhythm
  for(i=0; i<self->subdivisions_per_beat; i++)
    {
      float r = random() / (double)(RAND_MAX);
      float x = self->histogram[self->histogram_index + i];
      float nonliniarity = fabs(2*x-1);
      nonliniarity = pow(nonliniarity, self->nonlinear_exponent);
      nonliniarity *= self->is_inverse;
      if(x < 0.5) nonliniarity *= -1;
      nonliniarity += 1;
      nonliniarity /= 2.0;

      if(nonliniarity > r)
        {
          returned_rhythm[n].beat_time    = i/(float)self->subdivisions_per_beat;
          returned_rhythm[n].strength     = -1;
          returned_rhythm[n].timbre_class = -1;
          ++n;
        }
      if(self->experiment_is_running)
        fprintf(stderr, "%i\t", nonliniarity > r);
    }
  
  if(self->experiment_is_running)
    fprintf(stderr, "\r\n");
  
  return n;
}

