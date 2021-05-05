/*
 *  Synth.h
 *  Make weird noises
 *
 *  Made by Michael Krzyzaniak at Arizona State University's
 *  School of Arts, Media + Engineering in Spring of 2013
 *  mkrzyzan@asu.edu
 */

#include "Microphone.h"
#include "Click.h"
#include "Timestamp.h"

#include <pthread.h>


//also defined in main.c
//#define  ICLI_MODE 1

#ifdef   ICLI_MODE
#include "extras/OSC.h"
#include "extras/Network.h"
#define  OSC_BUFFER_SIZE 1024
#define  OSC_VALUES_BUFFER_SIZE 64
#define  OSC_SEND_PORT   9100
#define  OSC_RECV_PORT   9101
void*    mic_OSC_recv_thread_run_loop(void* SELF);
#endif

#define MIC_RHYTHM_THREAD_RUN_LOOP_INTERVAL 1000 //1000 usec, 1ms

void  mic_onset_detected_callback(void* SELF, unsigned long long sample_time);
void  mic_beat_detected_callback (void* SELF, unsigned long long sample_time);
void  mic_message_recd_from_robot(void* self, char* message, robot_arg_t args[], int num_args);

void* mic_rhythm_thread_run_loop (void* SELF);
Microphone* mic_destroy         (Microphone* self);

/*--------------------------------------------------------------------*/
struct OpaqueMicrophoneStruct
{
#ifndef ICLI_MODE
  AUDIO_GUTS               ;
#else
  Network* net;
  char* osc_send_buffer;
  char* osc_recv_buffer;
  oscValue_t* osc_values_buffer;
  pthread_t osc_recv_thread;
  Microphone* (*destroy)(Microphone*);
#endif

  BTT* btt                 ;
  Click*  click            ;
  Robot*  robot            ;
  int     silent_beat_count;
  int     count_out_n      ;
  
  int     play_beat_bell   ;
  
  Rhythm* rhythm;
  int     rhythm_constructor_index;
  
  rhythm_onset_t*  rhythm_onsets;
  int     rhythm_onsets_len;
  int     num_rhythm_onsets;
  int     rhythm_onsets_index;
  
  pthread_t rhythm_thread;
  int     rhythm_thread_run_loop_running;
  
  pthread_mutex_t rhythm_generator_swap_mutex;
  pthread_mutex_t rhythm_mutex;

  int      farey_order;
  
  unsigned beat_clock;
  unsigned long long thread_clock;
  
};

/*--------------------------------------------------------------------*/
Microphone* mic_new()
{
#ifndef   ICLI_MODE
  Microphone* self = (Microphone*) auAlloc(sizeof(*self), mic_audio_callback, NO, 2, 44100, 128, 6);
#else
  Microphone* self = (Microphone*) calloc(1, sizeof(*self));
#endif

  if(self != NULL)
    {
#ifndef   ICLI_MODE
      self->destroy = (Audio* (*)(Audio*))mic_destroy;
#else
      self->destroy = mic_destroy;
#endif
      self->click = click_new();
      //if(self->click == NULL)
        //return (Microphone*)auDestroy((Audio*)self);
    
      self->robot = robot_new(mic_message_recd_from_robot, self);
      if(self->robot == NULL)
        return (Microphone*)auDestroy((Audio*)self);
 
 #ifndef   ICLI_MODE
      self->btt = btt_new_default();
      btt_set_log_gaussian_tempo_weight_width(self->btt, 10000);
#else
      //self->btt = btt_new(int spectral_flux_stft_len, int spectral_flux_stft_overlap, int oss_filter_order, int oss_length, int cbss_length, int onset_threshold_len, double sample_rate);
      self->btt = btt_new(32 /*BTT_SUGGESTED_SPECTRAL_FLUX_STFT_LEN*/,
                          2   /*BTT_SUGGESTED_SPECTRAL_FLUX_STFT_OVERLAP*/,
                          9   /*BTT_SUGGESTED_OSS_FILTER_ORDER*/,
                          40  /*BTT_SUGGESTED_OSS_LENGTH*/,
                          40  /*BTT_SUGGESTED_CBSS_LENGTH*/,
                          40 /*BTT_SUGGESTED_ONSET_THRESHOLD_N*/,
                          200 /*BTT_SUGGESTED_SAMPLE_RATE*/);
      btt_set_onset_threshold_min(self->btt, 0.150000);
      btt_set_num_tempo_candidates(self->btt, 5);
      btt_set_gaussian_tempo_histogram_width(self->btt, 0.5);
      btt_set_gaussian_tempo_histogram_decay(self->btt, 0.995);
      btt_set_log_gaussian_tempo_weight_width(self->btt, 10000);
#endif
      if(self->btt == NULL)
        return (Microphone*)auDestroy((Audio*)self);
    
      self->rhythm_onsets_len = 128;
      self->rhythm_onsets = calloc(self->rhythm_onsets_len, sizeof(*self->rhythm_onsets));
      if(self->rhythm_onsets == NULL)
        return (Microphone*)auDestroy((Audio*)self);

      btt_set_onset_tracking_callback  (self->btt, mic_onset_detected_callback, self);
      btt_set_beat_tracking_callback   (self->btt, mic_beat_detected_callback , self);
    
      if(pthread_mutex_init(&self->rhythm_generator_swap_mutex, NULL) != 0)
        return (Microphone*)auDestroy((Audio*)self);

      if(pthread_mutex_init(&self->rhythm_mutex, NULL) != 0)
        return (Microphone*)auDestroy((Audio*)self);

      //should happen after mutex init
      self->rhythm = mic_set_rhythm_generator_index(self, 0);
    
      self->rhythm_thread_run_loop_running = 1;
      int error = pthread_create(&self->rhythm_thread, NULL, mic_rhythm_thread_run_loop, self);
      if(error != 0)
        return (Microphone*)auDestroy((Audio*)self);
      
      mic_set_should_play_beat_bell   (self, 1);
      mic_set_quantization_order(self, 8);
      mic_set_count_out_n(self, 8);
    
#ifdef   ICLI_MODE
      self->osc_send_buffer = calloc(1, OSC_BUFFER_SIZE);
      if(!self->osc_send_buffer) return (Microphone*)auDestroy((Audio*)self);
      self->osc_recv_buffer = calloc(1, OSC_BUFFER_SIZE);
      if(!self->osc_recv_buffer) return (Microphone*)auDestroy((Audio*)self);
      self->osc_values_buffer = calloc(sizeof(*self->osc_values_buffer), OSC_VALUES_BUFFER_SIZE);
      if(!self->osc_values_buffer) return (Microphone*)auDestroy((Audio*)self);
      self->net  = net_new();
      if(!self->net) return (Microphone*)auDestroy((Audio*)self);
      if(!net_udp_connect (self->net, OSC_RECV_PORT))
        return (Microphone*)auDestroy((Audio*)self);
      error = pthread_create(&self->osc_recv_thread, NULL, mic_OSC_recv_thread_run_loop, self);
      if(error != 0)
        return (Microphone*)auDestroy((Audio*)self);
#endif

  //there should be a play callback that I can intercept and do this there.
      sleep(1);
      robot_send_message(self->robot, robot_cmd_get_firmware_version);
      if(self->click != NULL)
        auPlay((Audio*)self->click);
    }
  return self;
}

/*--------------------------------------------------------------------*/
void mic_onset_detected_callback(void* SELF, unsigned long long sample_time)
{
  Microphone* self = (Microphone*) SELF;
  
  pthread_mutex_lock(&self->rhythm_generator_swap_mutex);
  if(self->rhythm != NULL)
    rhythm_onset(self->rhythm, self->btt, sample_time);
  pthread_mutex_unlock(&self->rhythm_generator_swap_mutex);
  
  //if(btt_get_tracking_mode(self->btt) <= BTT_ONSET_TRACKING)
    //{
      //if(self->play_beat_bell)
        //{
          //if(self->click != NULL)
          //  click_click(self->click, 0.5);
          //robot_send_message(self->robot, robot_cmd_tap, 1.0 /*strength*/);
          //fprintf(stderr, "onset\r\n");
        //}
    //}

  self->silent_beat_count = 0;
}

/*--------------------------------------------------------------------*/
void mic_beat_detected_callback (void* SELF, unsigned long long sample_time)
{
  Microphone* self = (Microphone*) SELF;
  
  //mutex lock
  
  pthread_mutex_lock(&self->rhythm_mutex);
  self->rhythm_onsets_index = 0;
  int i;
  pthread_mutex_lock(&self->rhythm_generator_swap_mutex);
  self->rhythm_onsets_index = 0;
  if(self->rhythm != NULL)
    self->num_rhythm_onsets = rhythm_beat(self->rhythm, self->btt, sample_time, self->rhythm_onsets, self->rhythm_onsets_len);
  float beat_period = btt_get_beat_period_audio_samples(self->btt) / (float)btt_get_sample_rate(self->btt);
  pthread_mutex_unlock(&self->rhythm_generator_swap_mutex);
  
  
  //fprintf(stderr, "[");
  for(i=0; i<self->num_rhythm_onsets; i++)
    {
      rhythm_onset_t* onset = &self->rhythm_onsets[i];
      int num, denom;
      rhythm_get_rational_approximation(onset->beat_time, self->farey_order, &num, &denom);
      if(onset->strength < 0)
        onset->strength = 1.0 / denom;
      if(1 /*self->should_quantize*/)
        onset->beat_time = (float)num / (float)denom;
    
      onset->beat_time *= round(beat_period * (1000000 / (double)MIC_RHYTHM_THREAD_RUN_LOOP_INTERVAL));
      //fprintf(stderr, " %i: %f ", i, onset->beat_time);
    }
  //fprintf(stderr, "]\r\n");
  self->beat_clock = 0;
  pthread_mutex_unlock(&self->rhythm_mutex);
  
  if(self->play_beat_bell)
    {
      if(self->click != NULL)
        click_klop(self->click, 0.5);
      robot_send_message(self->robot, robot_cmd_bell, 1.0 /*strength*/);
      //fprintf(stderr, "beat\r\n");
    }
  
  if(self->count_out_n > 1)
    {
      ++self->silent_beat_count;
      if(self->silent_beat_count > self->count_out_n)
        btt_set_tracking_mode(self->btt, BTT_COUNT_IN_TRACKING);
    }
  
  robot_send_message(self->robot, robot_cmd_eye_blink);
}

/*--------------------------------------------------------------------*/
Microphone* mic_destroy(Microphone* self)
{
  if(self != NULL)
    {
      if(self->rhythm_thread != 0)
        {
          self->rhythm_thread_run_loop_running = 0;
          pthread_join(self->rhythm_thread, NULL);
        
#ifdef ICLI_MODE
          pthread_cancel(self->osc_recv_thread);
          pthread_join(self->osc_recv_thread, NULL);
          self->net = net_destroy(self->net);
          if(self->osc_recv_buffer != NULL)
            free(self->osc_recv_buffer);
          if(self->osc_send_buffer != NULL)
            free(self->osc_send_buffer);
          if(self->osc_values_buffer != NULL)
            free(self->osc_values_buffer);
#endif
        }
    
      btt_destroy(self->btt);
      robot_destroy(self->robot);
      auDestroy((Audio*)self->click);
    }
    
  return (Microphone*) NULL;
}

/*--------------------------------------------------------------------*/
BTT*           mic_get_btt        (Microphone* self)
{
  return self->btt;
}

/*--------------------------------------------------------------------*/
Robot*            mic_get_robot                  (Microphone* self)
{
  return self->robot;
}

/*--------------------------------------------------------*/
void mic_message_recd_from_robot(void* self, char* message, robot_arg_t args[], int num_args)
{
  //robot_debug_print_message(message, args, num_args);
  switch(robot_hash_message(message))
    {
      case robot_hash_reply_firmware_version:
        if(num_args == 2)
          fprintf(stderr, "Teensy is running firmware version %i.%i\r\n", robot_arg_to_int(&args[0]), robot_arg_to_int(&args[1]));
        break;
      
      default: break;
    }
}

/*--------------------------------------------------------------------*/
Rhythm* mic_set_rhythm_generator       (Microphone* self, rhythm_new_funct constructor)
{
  if(constructor != rhythm_constructors[self->rhythm_constructor_index])
    {
      int i;
      self->rhythm_constructor_index = -1;
      for(i=0; i<rhythm_num_constructors; i++)
        if(constructor == rhythm_constructors[i])
          {self->rhythm_constructor_index = i; break;}
  
      pthread_mutex_lock(&self->rhythm_generator_swap_mutex);
      if(self->rhythm != NULL)
        self->rhythm = rhythm_destroy(self->rhythm);
      self->rhythm = (constructor == NULL) ? NULL : constructor(self->btt);
      self->num_rhythm_onsets = 0;
      pthread_mutex_unlock(&self->rhythm_generator_swap_mutex);
    }
  return self->rhythm;
}

/*--------------------------------------------------------------------*/
Rhythm* mic_set_rhythm_generator_index (Microphone* self, int index)
{
  if(rhythm_num_constructors <= 0) return self->rhythm;
  
  if(index < 0) index = 0;
  if(index >= rhythm_num_constructors) index = rhythm_num_constructors-1;
  
  return mic_set_rhythm_generator(self, rhythm_constructors[index]);
}

/*--------------------------------------------------------------------*/
int mic_get_rhythm_generator_index (Microphone* self)
{
  return self->rhythm_constructor_index;
}

/*--------------------------------------------------------------------*/
Rhythm* mic_get_rhythm_generator       (Microphone* self)
{
  return self->rhythm;
}

/*--------------------------------------------------------------------*/
void              mic_set_should_play_beat_bell   (Microphone* self, int should)
{
  self->play_beat_bell = (should != 0);
}

/*--------------------------------------------------------------------*/
int               mic_get_should_play_beat_bell   (Microphone* self)
{
  return (self->play_beat_bell != 0);
}

/*--------------------------------------------------------------------*/
void              mic_set_quantization_order(Microphone* self, int order)
{
  if(order < 0) order = 0;
  self->farey_order = order;
}

/*--------------------------------------------------------------------*/
int               mic_get_quantization_order(Microphone* self)
{
  return self->farey_order;
}

/*--------------------------------------------------------------------*/
void              mic_set_count_out_n            (Microphone* self, int n)
{
  if(n < 0) n = 0;
  self->count_out_n = n;
}

/*--------------------------------------------------------------------*/
int               mic_get_count_out_n            (Microphone* self)
{
  return self->count_out_n;
}

/*--------------------------------------------------------------------*/
void* mic_rhythm_thread_run_loop (void* SELF)
{
  Microphone* self = (Microphone*)SELF;
  timestamp_microsecs_t start = timestamp_get_current_time();
  
  while(self->rhythm_thread_run_loop_running)
    {
      pthread_mutex_lock(&self->rhythm_mutex);
      while(self->rhythm_onsets_index < self->num_rhythm_onsets)  
        if(self->beat_clock >= self->rhythm_onsets[self->rhythm_onsets_index].beat_time)
          {
            int timbre = self->rhythm_onsets[self->rhythm_onsets_index].timbre_class;
            if(self->click != NULL)
              click_click(self->click, self->rhythm_onsets[self->rhythm_onsets_index].strength);
            if(timbre < 0)
              robot_send_message(self->robot, robot_cmd_tap, self->rhythm_onsets[self->rhythm_onsets_index].strength);
            else
              robot_send_message(self->robot, robot_cmd_tap_specific, timbre, self->rhythm_onsets[self->rhythm_onsets_index].strength);
            ++self->rhythm_onsets_index;
          }
        else
          break;
      //ignore any other (duplicate) onsets that are supposed to happen right now
      //jk, allow multiple simulataneous onsets. todo -- only allow multiple tap-specific messages
      //stable-sort by timbre class then by onset time, ignore multiple tap-specific for same solenoid, etc
      /*
      while(self->rhythm_onsets_index < self->num_rhythm_onsets)
        {
          if(self->beat_clock >= self->rhythm_onsets[self->rhythm_onsets_index].beat_time)
            ++self->rhythm_onsets_index;
          else break;
        }
      */
      ++self->beat_clock; //reset at the begining of each beat
      ++self->thread_clock; //never reset
      pthread_mutex_unlock(&self->rhythm_mutex);
    
      while((timestamp_get_current_time() - start) < self->thread_clock*MIC_RHYTHM_THREAD_RUN_LOOP_INTERVAL)
        usleep(50);
    }
  return NULL;
}

#ifdef ICLI_MODE
/*--------------------------------------------------------------------*/
void* mic_OSC_recv_thread_run_loop(void* SELF)
{
  Microphone* self = (Microphone*)SELF;
  char senders_address[16];
  char *osc_address, *osc_type_tag;
  
  for(;;)
  {
    int num_valid_bytes = net_udp_receive (self->net, self->osc_recv_buffer, OSC_BUFFER_SIZE, senders_address);
    if(num_valid_bytes < 0)
      continue; //return NULL ?
  
    int num_osc_values = oscParse(self->osc_recv_buffer, num_valid_bytes, &osc_address, &osc_type_tag, self->osc_values_buffer, OSC_VALUES_BUFFER_SIZE);
    if(num_osc_values < 1)
        continue;
  
    uint32_t address_hash = oscHash((unsigned char*)osc_address);
    if(address_hash == 2085476677) // '/emg'
      {
        // /emg <float> <float> <float> ...
        //fprintf(stderr, "num_values: %i\r\n", num_osc_values);
      
        dft_sample_t buffer[num_osc_values];
        int i;
        for(i=0; i<num_osc_values; i++)
          buffer[i] = oscValueAsFloat(self->osc_values_buffer[i], osc_type_tag[i]);
      
        btt_process(self->btt, buffer, num_osc_values);
      }
  }
}

#else  //!ICLI_MODE
/*--------------------------------------------------------------------*/
int mic_audio_callback(void* SELF, auSample_t* buffer, int num_frames, int num_channels)
{
  Microphone* self = (Microphone*)SELF;
  int frame, channel;
  auSample_t samp = 0;
  
  //mix to mono without correcting amplitude
  for(frame=0; frame<num_frames; frame++)
    {
      samp = 0;
      for(channel=0; channel<num_channels; channel++)
        samp += buffer[frame * num_channels + channel];
      buffer[frame] = samp;
  }

  btt_process(self->btt, buffer, num_frames);

  return  num_frames;
}
#endif //ICLI_MODE

