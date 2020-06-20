#include "Eye.h"
#include "List.h"

#include "Wire.h"
//#include "Adafruit_GFX.h"
//#include "Adafruit_IS31FL3731.h"

/* eye is powered by 3.3 V */
/* draws 20 mA at brightness 30 */
/* draws 100 mA at brightness 255 */
/* eye update loop takesnabout 15 ms to run (mostly consumed by i2c) */


/* -------------------------------------------------------------- */
typedef enum eye_param_indices_enum
{
  EYE_WIDTH           = 0,
  EYE_HEIGHT             ,
  EYE_POSITION_X         ,
  EYE_POSITION_Y         ,
  EYE_IRIS_WIDTH         ,
  EYE_IRIS_HEIGHT        ,
  EYE_IRIS_POSITION_X    ,
  EYE_IRIS_POSITION_Y    ,
  EYE_NUM_PARAMS         ,
}eye_param_t;

float    EYE_DEFAULT_VALS[] =     {12, 9, 0, 0, 6, 2, 0, 0};
#define  EYE_INIT_USE_DEFAULTS    -128
#define  EYE_NULL_VALUE           -127
#define  EYE_LED_WIDTH             16
#define  EYE_LED_HEIGHT            9

#define EYE_UNCONNECTED_ANALOG_PIN 0   //for seeding random
#define EYE_BRIGHTNESS             30  //max is 255
#define EYE_UPDATE_INTERVAL        33  //millisecs
#define EYE_SACCADE_TIMER_DURATION 2000 /*milliseconds*/ / EYE_UPDATE_INTERVAL

/* -------------------------------------------------------------- */
typedef struct opaque_eye_struct
{
  float c[EYE_NUM_PARAMS];
}Eye;

/* -------------------------------------------------------------- */
Eye* eye_new(float init_val);
void eye_draw(Eye* eye);
void eye_animate_single_param(eye_param_t param, int val, float millisecs);
void eye_setup_IS31FL3731(void);
void eye_animate_run_loop(void);

/* -------------------------------------------------------------- */
/* GLOBAL VARIABLES */
IntervalTimer       eye_timer_thread;
List*               eye_global_queues;
Eye*                eye_global_eye;

int                 eye_global_saccade_resume_timer;
int                 eye_global_is_initalized;

/* -------------------------------------------------------------- */
void eye_init_module()
{
  eye_global_is_initalized = 0;
  eye_setup_IS31FL3731();
  eye_global_queues = list_new();
  eye_global_eye    = eye_new(EYE_INIT_USE_DEFAULTS);
  eye_global_saccade_resume_timer = 0;
  randomSeed(analogRead(EYE_UNCONNECTED_ANALOG_PIN));
  eye_timer_thread.priority(128);
  eye_timer_thread.begin(eye_animate_run_loop, EYE_UPDATE_INTERVAL * 1000);
  eye_animate_roll(2);
}

/* -------------------------------------------------------------- */
Eye* eye_new(float init_val)
{
  Eye* self = (Eye*)calloc(1, sizeof(*self));
  if(self != NULL)
    {
      int i;
      if(init_val == EYE_INIT_USE_DEFAULTS)
        {
          float* defaults = EYE_DEFAULT_VALS;
          for(i=0; i<EYE_NUM_PARAMS; i++)
            self->c[i] = defaults[i];
        }
      else
        for(i=0; i<EYE_NUM_PARAMS; i++)
          self->c[i] = init_val;
    }
  return self;
}

/* -------------------------------------------------------------- */
Eye* eye_copy(Eye* eye)
{
  Eye* self = (Eye*)calloc(1, sizeof(*self));
  if(self != NULL)
    {
      int i;
      for(i=0; i<EYE_NUM_PARAMS; i++)
        self->c[i] = eye->c[i];
    }
  return self;
}

/* -------------------------------------------------------------- */
Eye* eye_destroy(Eye* eye)
{
  if(eye != NULL)
    {
      free(eye);
    }
  return (Eye*) NULL;
}

/* -------------------------------------------------------------- */
int eye_max(int a, int b)
{
  return (a > b ) ? a : b;
}

/* -------------------------------------------------------------- */
int eye_min(int a, int b)
{
  return (a > b ) ? b : a;
}

/* -------------------------------------------------------------- */
float eye_random()
{
  return random(1000000) / (long double)1000000.0;
}

/* -------------------------------------------------------------- */
void eye_draw_pixel(int x, int  y, float val, unsigned char pixel_buffer[])
{
  if((x >= 0) && (x < 16))
    if((y >= 0) && (y < 9))
       pixel_buffer[y * EYE_LED_WIDTH + x] = val;  
}

/* -------------------------------------------------------------- */
void eye_setup_IS31FL3731()
{
  
  Wire.setClock(400000);
  Wire.begin();

  //switch to page 9
  Wire.beginTransmission(0x74);
  Wire.write((byte) 0xFD);
  Wire.write((byte) 0x0B);
  Wire.endTransmission();

  //make sure the byte we just wrote 'stuck'
  //otherwise the board is not powered on.
  Wire.beginTransmission(0x74);
  Wire.write((byte) 0xFD);
  Wire.endTransmission();
  
  Wire.requestFrom(0x74, 1, true);
  if(Wire.available())
    if(Wire.read() == 0x0B)
      eye_global_is_initalized = 1;
  if(eye_global_is_initalized != 1)
    return;
  
  //go into shutdown mode
  Wire.beginTransmission(0x74);
  Wire.write((byte) 0x0A);
  Wire.write((byte) 0x00);
  Wire.endTransmission();

  delay(10);

  //picture mode
  Wire.beginTransmission(0x74);
  Wire.write((byte) 0x00);
  Wire.write((byte) 0x00);
  Wire.endTransmission();

  //display frame 0
  Wire.beginTransmission(0x74);
  Wire.write((byte) 0x01);
  Wire.write((byte) 0x00);
  Wire.endTransmission();

  //turn audio sync off
  Wire.beginTransmission(0x74);
  Wire.write((byte) 0x06);
  Wire.write((byte) 0x00);
  Wire.endTransmission();

  //enable all leds on in all frames and set PWM to 0
  int frame, reg;
  for (frame=0; frame<8; frame++)
    {
      //switch to frame 'frame'
      Wire.beginTransmission(0x74);
      Wire.write((byte) 0xFD);
      Wire.write((byte) frame);
      Wire.endTransmission();

      /* set PWM to 0 for each pixel in this frame */
      int i, j;
      for(i=0; i<6; i++)
        {
          Wire.beginTransmission(0x74);
          Wire.write((byte) 0x24 + 24*i);
          for(j=0; j<24; j++)
            Wire.write((byte) 0);
          Wire.endTransmission();
        }
      /* turn LED on for each pixel in this frame */
      for (reg=0; reg<=0x11; reg++)
        {
          Wire.beginTransmission(0x74);
          Wire.write((byte) reg);
          Wire.write((byte) 0xFF);
          Wire.endTransmission(); 
        }
    }

  //switch back to page 9
  Wire.beginTransmission(0x74);
  Wire.write((byte) 0xFD);
  Wire.write((byte) 0x0B);
  Wire.endTransmission();
  
  //come out of shutdown mode
  Wire.beginTransmission(0x74);
  Wire.write((byte) 0x0A);
  Wire.write((byte) 0x01);
  Wire.endTransmission();
  
  //switch back to page 0 for animating to frame 0
  Wire.beginTransmission(0x74);
  Wire.write((byte) 0xFD);
  Wire.write((byte) 0x00);
  Wire.endTransmission();
}

/* -------------------------------------------------------------- */
void eye_draw(Eye* eye)
{
  int x, y;
  unsigned char pixel_buffer[EYE_LED_WIDTH * EYE_LED_HEIGHT] = {0};

  float center_x = EYE_LED_WIDTH  * 0.5;
  float center_y = EYE_LED_HEIGHT * 0.5;

  float half_height = 0.5 * eye->c[EYE_HEIGHT];
  float half_width  = 0.5 * eye->c[EYE_WIDTH];

  float top = round(center_y - half_height + eye->c[EYE_POSITION_Y]);
  float bottom = top + eye->c[EYE_HEIGHT];

  float left = round(center_x - half_width + eye->c[EYE_POSITION_X]);
  float right = left + eye->c[EYE_WIDTH];

  float top_iris = round(center_y - (0.5*eye->c[EYE_IRIS_HEIGHT]) + eye->c[EYE_POSITION_Y] + eye->c[EYE_IRIS_POSITION_Y]);
  top_iris = eye_max(top_iris, top);
  top_iris = eye_min(top_iris, bottom-eye->c[EYE_IRIS_HEIGHT]);
  float bottom_iris = top_iris + eye->c[EYE_IRIS_HEIGHT];
  bottom_iris = eye_min(bottom_iris, bottom);

  float left_iris = round(center_x  + eye->c[EYE_POSITION_X] + eye->c[EYE_IRIS_POSITION_X]  - (0.5*eye->c[EYE_IRIS_WIDTH])) ;
  left_iris = eye_max(left_iris, left);
  left_iris = eye_min(left_iris, right-eye->c[EYE_IRIS_WIDTH]);
  float right_iris = left_iris + eye->c[EYE_IRIS_WIDTH];
  right_iris = eye_min(right_iris, right);

  //draw rectangular eye
  for(y=top; y<bottom; y++) //top to bottom
    for(x=left; x<right; x++) //left to right
      eye_draw_pixel(x, y, EYE_BRIGHTNESS, pixel_buffer);

  //take out corners from eye
  if(eye->c[EYE_HEIGHT] > 2)
    {
      if(eye->c[EYE_WIDTH] > 1)
        eye_draw_pixel(right-1, top, 0, pixel_buffer);
      if(eye->c[EYE_WIDTH] > 2)
        eye_draw_pixel(left, top, 0, pixel_buffer);
    }
  if(eye->c[EYE_HEIGHT] > 1)
    {
      if(eye->c[EYE_WIDTH] > 1)
        eye_draw_pixel(right-1, bottom-1, 0, pixel_buffer);
      if(eye->c[EYE_WIDTH] > 2)
        eye_draw_pixel(left, bottom-1, 0, pixel_buffer);
    }
  if((eye->c[EYE_WIDTH] > 4) && (eye->c[EYE_HEIGHT] > 4))
    {
      //4%, 56%, 85%
      eye_draw_pixel(right-2, top, EYE_BRIGHTNESS * 0.05, pixel_buffer);// 25%
      eye_draw_pixel(left+1, top, EYE_BRIGHTNESS * 0.05, pixel_buffer);
      eye_draw_pixel(right-1, top+1, EYE_BRIGHTNESS * 0.05, pixel_buffer);
      eye_draw_pixel(left, top+1, EYE_BRIGHTNESS * 0.05, pixel_buffer);
      eye_draw_pixel(right-2, bottom-1, EYE_BRIGHTNESS * 0.05, pixel_buffer);
      eye_draw_pixel(left+1, bottom-1, EYE_BRIGHTNESS * 0.05, pixel_buffer);
      eye_draw_pixel(right-1, bottom-2, EYE_BRIGHTNESS * 0.05, pixel_buffer);
      eye_draw_pixel(left, bottom-2, EYE_BRIGHTNESS * 0.05, pixel_buffer);
            
      eye_draw_pixel(right-3, top, EYE_BRIGHTNESS * 0.3, pixel_buffer);
      eye_draw_pixel(left+2, top, EYE_BRIGHTNESS * 0.3, pixel_buffer);
      eye_draw_pixel(right-1, top+2, EYE_BRIGHTNESS * 0.3, pixel_buffer);
      eye_draw_pixel(left, top+2, EYE_BRIGHTNESS * 0.3, pixel_buffer);
      eye_draw_pixel(right-3, bottom-1, EYE_BRIGHTNESS * 0.3, pixel_buffer);
      eye_draw_pixel(left+2, bottom-1, EYE_BRIGHTNESS * 0.3, pixel_buffer);
      eye_draw_pixel(right-1, bottom-3, EYE_BRIGHTNESS * 0.3, pixel_buffer);
      eye_draw_pixel(left, bottom-3, EYE_BRIGHTNESS * 0.3, pixel_buffer);
            
      eye_draw_pixel(right-2, top+1, EYE_BRIGHTNESS * 0.75, pixel_buffer);
      eye_draw_pixel(left+1, top+1, EYE_BRIGHTNESS * 0.75, pixel_buffer);
      eye_draw_pixel(right-2, bottom-2, EYE_BRIGHTNESS * 0.75, pixel_buffer);
      eye_draw_pixel(left+1, bottom-2, EYE_BRIGHTNESS * 0.75, pixel_buffer);
    }   

  //irises
  for(y=top_iris; y<bottom_iris; y++) //top to bottom
    {
      for(x=left_iris; x<right_iris; x++) //left to right
        eye_draw_pixel(x, y, 0, pixel_buffer);
    }

  //for unknown reasons we have to write blocks of at most 24 bytes at a time
  //this takes 15 ms, could be closer to 5 ms it we could write all in one transmission
  int i=0;
  for(x=0; x<6; x++)
    {
      Wire.beginTransmission(0x74);
      Wire.write((byte) 0x24 + i);
      for(y=0; y<24; y++)
        Wire.write((byte) pixel_buffer[i++]);
      Wire.endTransmission();
    }
}

/* -------------------------------------------------------------- */
/* -------------------------------------------------------------- */
/* -------------------------------------------------------------- */
/* USER CODE FOR ANIMATING EYES */
float eye_animation_scalef(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/* -------------------------------------------------------------- */
//duration of first pose is ignored
typedef struct opaque_eye_animation_pose_struct
{
  Eye* eye;
  int steps;
}Eye_Animation_Pose;

/* -------------------------------------------------------------- */
Eye_Animation_Pose* eye_animation_pose_new(Eye* eye, int duration_millisecs)
{
  Eye_Animation_Pose* self = (Eye_Animation_Pose*)calloc(1, sizeof(*self));
  if(self != NULL)
    {
      self->eye = eye;
      self->steps = eye_max(1, round(duration_millisecs / (float)EYE_UPDATE_INTERVAL));
    }
  return self;
}

/* -------------------------------------------------------------- */
Eye_Animation_Pose* eye_animation_pose_destroy(Eye_Animation_Pose* self)
{
  if(self != NULL)
    {
      self->eye = eye_destroy(self->eye);
      free(self);
    }
  return (Eye_Animation_Pose*) NULL;
}

/* -------------------------------------------------------------- */
//typedef struct
typedef struct opaque_eye_animation_queue_struct
{
  List* poses;
  int pose;
  int pose_timer;
  int should_loop;
}Eye_Animation_Queue;

/* -------------------------------------------------------------- */
Eye_Animation_Queue* eye_animation_queue_new(List* poses, int should_loop)
{
  Eye_Animation_Queue* self = (Eye_Animation_Queue*)calloc(1, sizeof(*self));
  if(self != NULL)
    {
      self->poses        = poses;
      self->pose         = 0;
      self->pose_timer   = 0;
      self->should_loop  = should_loop;
    }
  return self;
}

/* -------------------------------------------------------------- */
Eye_Animation_Queue* eye_animation_queue_destroy(Eye_Animation_Queue* self)
{
  if(self != NULL)
    {
      list_destroy(self->poses, YES);
      free(self);
    }
  return (Eye_Animation_Queue*) NULL;
}

/* -------------------------------------------------------------- */
void eye_animate_run_loop(void)
{  
  Eye* eye = eye_global_eye;
  List* queues = eye_global_queues;

  if(eye_global_is_initalized != 1)
    {
       eye_setup_IS31FL3731();
       return;
    }
  
  list_iterator_t iter = list_reset_iterator(queues);
  Eye_Animation_Queue* queue;
  
  while((queue = (Eye_Animation_Queue*)list_advance_iterator(queues, &iter)) != NULL)
  {
      if(queue == NULL) continue;
      if(list_count(queue->poses) > queue->pose+1 /* what about loop mode? */)
        {
          ++queue->pose_timer;
          Eye_Animation_Pose* self_pose = (Eye_Animation_Pose*)list_data_at_index(queue->poses, queue->pose);
          Eye_Animation_Pose* next_pose = (Eye_Animation_Pose*)list_data_at_index(queue->poses, queue->pose+1);
          int steps = next_pose->steps;

          if((self_pose == NULL) || (next_pose == NULL)) continue;
          if((self_pose->eye == NULL) || (next_pose->eye == NULL)) continue;
          
          int param;
          for(param=0; param<EYE_NUM_PARAMS; param++)
            {
              //interpolate robot eye
              if((self_pose->eye->c[param] != EYE_NULL_VALUE) && (next_pose->eye->c[param] != EYE_NULL_VALUE))
                eye->c[param] = round(eye_animation_scalef(queue->pose_timer, 0, steps, self_pose->eye->c[param], next_pose->eye->c[param]));
            }
          if(queue->pose_timer >= steps)
            {
              ++queue->pose;
              queue->pose_timer = 0;
            }
        }
      else
        {
          //if should_loop, loop, damnit!
          noInterrupts();
          list_remove_data(queues, queue, YES);
          interrupts();
        }
  }

  eye_draw(eye);

  if(false)
  //if(eye_global_saccade_resume_timer <= 0)
    {
      float r;
      r = eye_random();  
      // typical blink rate of adults is every 4 seconds
      // but this is too busy looking
      if(r < (EYE_UPDATE_INTERVAL / (6.0 * 1000.0)))
        eye_animate_blink();

        
      r = eye_random();
      if(r < (EYE_UPDATE_INTERVAL / (1.5 * 1000.0)))
        {
          float xx = 4*eye_random();
          eye_animate_single_param(EYE_IRIS_POSITION_X, floor(xx) - 1, 200);
          eye_animate_single_param(EYE_POSITION_X     , floor(xx) - 1, 200);
        }

      r = eye_random();
      if(r < (EYE_UPDATE_INTERVAL / (1.5 * 1000.0)))
        {
          float yy = 3*eye_random();
          eye_animate_single_param(EYE_IRIS_POSITION_Y, 2 * (floor(yy) - 1), 200);
          eye_animate_single_param(EYE_POSITION_Y     , floor(yy) - 1, 200);
        }

      /*
      r = eye_random();
      if(r < (EYE_UPDATE_INTERVAL / (60 * 1000.0)))
        eye_animate_shifty(4, 100);
     */
     
      //not completely thread safe, but issues should correct themselves on next run loop
      eye_global_saccade_resume_timer = 0;
    }
  else
    --eye_global_saccade_resume_timer;
}

/* -------------------------------------------------------------- */
void eye_initalize_initial_eye_hahaha(Eye* eye, Eye* initial_eye, Eye* target_eye)
{
  int param;
  for(param=0; param<EYE_NUM_PARAMS; param++)
    {
      if(target_eye->c[param] != EYE_NULL_VALUE)
        initial_eye->c[param] = eye->c[param];
    }
}

/* -------------------------------------------------------------- */
void eye_initalize_final_eye_hahaha(Eye* eye, Eye* initial_eye, Eye* final_eye)
{
  int param;
  float* defaults = EYE_DEFAULT_VALS;

  for(param=0; param<EYE_NUM_PARAMS; param++)
    {
      if(initial_eye->c[param] != EYE_NULL_VALUE)
        final_eye->c[param] = defaults[param];
    }
}

/* -------------------------------------------------------------- */
//update defaults?
void eye_go_to_pose(Eye* eye, List* queues, Eye* target_eye, float go_duration)
{
  Eye* initial_eye = eye_new(EYE_NULL_VALUE);
  eye_initalize_initial_eye_hahaha(eye, initial_eye, target_eye);

  List* poses = list_new();
  if(poses == NULL) return;
  Eye_Animation_Pose* pose = eye_animation_pose_new(initial_eye, 0);
  if(pose == NULL){list_destroy(poses, YES); return;}
  list_append_data(poses, pose, (list_data_deallocator_t)eye_animation_pose_destroy);
  pose = eye_animation_pose_new(target_eye, go_duration);
  if(pose == NULL){list_destroy(poses, YES); return;}
  list_append_data(poses, pose, (list_data_deallocator_t)eye_animation_pose_destroy);

  Eye_Animation_Queue* queue = eye_animation_queue_new(poses, false);
  if(queue == NULL){list_destroy(poses, YES); return;}
  
  //insert at index 0 so it has lower priority
  noInterrupts();
  list_insert_data_at_index(queues, queue, 0, (list_data_deallocator_t) eye_animation_queue_destroy);
  interrupts();
}

/* -------------------------------------------------------------- */
void eye_go_to_pose_stay_and_return(Eye* eye, List* queues, Eye* target_eye, float go_duration, float stay_duration, float return_duration)
{
  Eye* initial_eye = eye_new(EYE_NULL_VALUE);
  eye_initalize_initial_eye_hahaha(eye, initial_eye, target_eye);
  Eye* final_eye = eye_new(EYE_NULL_VALUE);
  eye_initalize_final_eye_hahaha(eye, initial_eye, final_eye);

  List* poses = list_new();
  if(poses == NULL) return;
  Eye_Animation_Pose* pose = eye_animation_pose_new(initial_eye, 0);
  if(pose == NULL){list_destroy(poses, YES); return;}
  list_append_data(poses, pose, (list_data_deallocator_t)eye_animation_pose_destroy);
  pose = eye_animation_pose_new(target_eye, go_duration);
  if(pose == NULL){list_destroy(poses, YES); return;}
  list_append_data(poses, pose, (list_data_deallocator_t)eye_animation_pose_destroy);
  
  if(stay_duration > 0)
    {
      pose = eye_animation_pose_new(eye_copy(target_eye), stay_duration);
      if(pose == NULL){list_destroy(poses, YES); return;}
      list_append_data(poses, pose, (list_data_deallocator_t)eye_animation_pose_destroy);
    }
  pose = eye_animation_pose_new(final_eye, return_duration);
  if(pose == NULL){list_destroy(poses, YES); return;}
  list_append_data(poses, pose, (list_data_deallocator_t)eye_animation_pose_destroy);
  
  Eye_Animation_Queue* queue = eye_animation_queue_new(poses, false);
  if(queue == NULL){list_destroy(poses, YES); return;}
  
  noInterrupts();
  list_append_data(queues, queue, (list_data_deallocator_t) eye_animation_queue_destroy);
  interrupts();
}

/* -------------------------------------------------------------- */
void eye_go_to_poses_stay_and_return(Eye* eye, List* queues, int num_poses, Eye* target_eye[/*num_poses*/], float go_durations[/*num_poses + 1*/], float stay_durations[/*num_poses*/])
{
  Eye* initial_eye = eye_new(EYE_NULL_VALUE);
  eye_initalize_initial_eye_hahaha(eye, initial_eye, target_eye[0]);
  Eye* final_eye = eye_new(EYE_NULL_VALUE);
  eye_initalize_final_eye_hahaha(eye, initial_eye, final_eye);

  List* poses = list_new();
  if(poses == NULL) return;
  Eye_Animation_Pose* pose = eye_animation_pose_new(initial_eye, 0);
  if(pose == NULL){list_destroy(poses, YES); return;}
  list_append_data(poses, pose, (list_data_deallocator_t)eye_animation_pose_destroy);
 
  int i;
  for(i=0; i<num_poses; i++)
    {
      pose = eye_animation_pose_new(target_eye[i], go_durations[i]);
      if(pose == NULL){list_destroy(poses, YES); return;}
      list_append_data(poses, pose, (list_data_deallocator_t)eye_animation_pose_destroy);
      if(stay_durations[i] > 0)
        {
          pose = eye_animation_pose_new(eye_copy(target_eye[i]), stay_durations[i]);
          if(pose == NULL){list_destroy(poses, YES); return;}
          list_append_data(poses, pose, (list_data_deallocator_t)eye_animation_pose_destroy);
        }
    }

  pose = eye_animation_pose_new(final_eye, go_durations[i]);
  if(pose == NULL){list_destroy(poses, YES); return;}
  list_append_data(poses, pose, (list_data_deallocator_t)eye_animation_pose_destroy);

  Eye_Animation_Queue* queue = eye_animation_queue_new(poses, false);
  if(queue == NULL){list_destroy(poses, YES); return;}
  
  noInterrupts();
  list_append_data(queues, queue, (list_data_deallocator_t) eye_animation_queue_destroy);
  interrupts();
}

/* -------------------------------------------------------------- */
void eye_animate_blink()
{
  Eye* target_eye = eye_new(EYE_NULL_VALUE);
  if(target_eye == NULL) return;

  target_eye->c[EYE_HEIGHT]      = 1;
  target_eye->c[EYE_WIDTH]       = EYE_LED_WIDTH;
  target_eye->c[EYE_IRIS_HEIGHT] = 0;
  target_eye->c[EYE_IRIS_WIDTH]  = 0;
  target_eye->c[EYE_POSITION_Y]  = eye_global_eye->c[EYE_POSITION_Y]+2;

  if(eye_global_saccade_resume_timer <= 0)
    eye_animate_neutral_position();

  eye_go_to_pose_stay_and_return(eye_global_eye, eye_global_queues, target_eye, 66, 0, 66);

  eye_global_saccade_resume_timer = EYE_SACCADE_TIMER_DURATION;
}

/* -------------------------------------------------------------- */
void eye_animate_inquisitive()
{
  Eye* target_eye = eye_new(EYE_NULL_VALUE);
  if(target_eye == NULL) return;

  target_eye->c[EYE_HEIGHT]  = 7;
  target_eye->c[EYE_WIDTH]   = 6;
  target_eye->c[EYE_IRIS_HEIGHT]  = 2;
  target_eye->c[EYE_IRIS_WIDTH]   = 2;

  eye_go_to_pose_stay_and_return(eye_global_eye, eye_global_queues, target_eye, 200, 1000, 300);
}

/* -------------------------------------------------------------- */
void eye_animate_focused()
{
  Eye* target_eye = eye_new(EYE_NULL_VALUE);
  if(target_eye == NULL) return;

  target_eye->c[EYE_HEIGHT]  = 5;
  target_eye->c[EYE_WIDTH]   = 5;
  target_eye->c[EYE_IRIS_HEIGHT]  = 1;
  target_eye->c[EYE_IRIS_WIDTH]   = 1;

  eye_go_to_pose_stay_and_return(eye_global_eye, eye_global_queues, target_eye, 200, 1000, 300);
}

/* -------------------------------------------------------------- */
void eye_animate_surprised()
{
  Eye* target_eye = eye_new(EYE_NULL_VALUE);
  if(target_eye == NULL) return;

  target_eye->c[EYE_HEIGHT]  = 9;
  target_eye->c[EYE_WIDTH]   = 4;
  target_eye->c[EYE_IRIS_HEIGHT]  = 2;
  target_eye->c[EYE_IRIS_WIDTH]   = 2;

  eye_go_to_pose_stay_and_return(eye_global_eye, eye_global_queues, target_eye, 200, 600, 200);
}

/* -------------------------------------------------------------- */
void eye_animate_neutral_size()
{
  Eye* target_eye = eye_new(EYE_NULL_VALUE);
  if(target_eye == NULL) return;

  target_eye->c[EYE_HEIGHT]  = 8;
  target_eye->c[EYE_WIDTH]   = 7;
  target_eye->c[EYE_IRIS_HEIGHT]  = 6;
  target_eye->c[EYE_IRIS_WIDTH]   = 2;

  eye_go_to_pose(eye_global_eye, eye_global_queues, target_eye, 200);
}

/* -------------------------------------------------------------- */
void eye_animate_neutral_position()
{
  Eye* target_eye = eye_new(EYE_NULL_VALUE);
  if(target_eye == NULL) return;

  target_eye->c[EYE_POSITION_X]  = 0;
  target_eye->c[EYE_POSITION_Y]  = 0;

  target_eye->c[EYE_IRIS_POSITION_X]  = 0;
  target_eye->c[EYE_IRIS_POSITION_Y]  = 0;

  eye_go_to_pose(eye_global_eye, eye_global_queues, target_eye, 200);
}

/* -------------------------------------------------------------- */
void eye_animate_single_param(eye_param_t param, int val, float millisecs)
{
  Eye* target_eye = eye_new(EYE_NULL_VALUE);
  if(target_eye == NULL) return;
  
  target_eye->c[param]  = val;
  eye_go_to_pose(eye_global_eye, eye_global_queues, target_eye, millisecs);
  EYE_DEFAULT_VALS[param] = val;
}

/* -------------------------------------------------------------- */
void eye_animate_roll(int depth)
{
  Eye* target_eye_1 = eye_new(EYE_NULL_VALUE);
  Eye* target_eye_2 = eye_new(EYE_NULL_VALUE);
  Eye* target_eye_3 = eye_new(EYE_NULL_VALUE);
  Eye* target_eye_4 = eye_new(EYE_NULL_VALUE);
  
  if((target_eye_1==NULL) || (target_eye_2==NULL) || (target_eye_3==NULL) || (target_eye_4==NULL))
    {
      eye_destroy(target_eye_1);
      eye_destroy(target_eye_2);
      eye_destroy(target_eye_3);
      eye_destroy(target_eye_4);
      return;
    }

  target_eye_1->c[EYE_POSITION_X]       = depth;
  target_eye_1->c[EYE_POSITION_Y]       = depth;
  target_eye_1->c[EYE_IRIS_POSITION_X]  = depth;
  target_eye_1->c[EYE_IRIS_POSITION_Y]  = depth;

  target_eye_2->c[EYE_POSITION_X]       = -depth;
  target_eye_2->c[EYE_POSITION_Y]       = depth;
  target_eye_2->c[EYE_IRIS_POSITION_X]  = -depth;
  target_eye_2->c[EYE_IRIS_POSITION_Y]  = depth;

  target_eye_3->c[EYE_POSITION_X]       = -depth;
  target_eye_3->c[EYE_POSITION_Y]       = -depth;
  target_eye_3->c[EYE_IRIS_POSITION_X]  = -depth;
  target_eye_3->c[EYE_IRIS_POSITION_Y]  = -depth;

  target_eye_4->c[EYE_POSITION_X]       = depth;
  target_eye_4->c[EYE_POSITION_Y]       = -depth;
  target_eye_4->c[EYE_IRIS_POSITION_X]  = depth;
  target_eye_4->c[EYE_IRIS_POSITION_Y]  = -depth;

  Eye* target_eye[]  = {target_eye_1, target_eye_2, target_eye_3, target_eye_4, eye_copy(target_eye_1), eye_copy(target_eye_2), eye_copy(target_eye_3), eye_copy(target_eye_4), eye_copy(target_eye_1)};
  float go_durations[] = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100};
  float stay_durations[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

  eye_go_to_poses_stay_and_return(eye_global_eye, eye_global_queues, 9, target_eye, go_durations, stay_durations);
}

/* -------------------------------------------------------------- */
void eye_animate_no(int depth, float speed)
{
  Eye* target_eye_1 = eye_new(EYE_NULL_VALUE);
  Eye* target_eye_2 = eye_new(EYE_NULL_VALUE);
  
  if((target_eye_1==NULL) || (target_eye_2==NULL))
    {
      eye_destroy(target_eye_1);
      eye_destroy(target_eye_2);
      return;
    }

  target_eye_1->c[EYE_POSITION_X]       = depth;
  target_eye_1->c[EYE_IRIS_POSITION_X]  = depth;

  target_eye_2->c[EYE_POSITION_X]       = -depth;
  target_eye_2->c[EYE_IRIS_POSITION_X]  = -depth;

  Eye* target_eye[]      = {target_eye_1, target_eye_2, eye_copy(target_eye_1)};
  float go_durations[]   = {speed, 2*speed, 2*speed,speed};
  float stay_durations[] = {2*speed, 2*speed, 2*speed};

  eye_go_to_poses_stay_and_return(eye_global_eye, eye_global_queues, 3, target_eye, go_durations, stay_durations);
}

/* -------------------------------------------------------------- */
void eye_animate_yes(int depth, float speed)
{
  Eye* target_eye_1 = eye_new(EYE_NULL_VALUE);
  Eye* target_eye_2 = eye_new(EYE_NULL_VALUE);
  
  if((target_eye_1==NULL) || (target_eye_2==NULL))
    {
      eye_destroy(target_eye_1);
      eye_destroy(target_eye_2);
      return;
    }

  target_eye_1->c[EYE_POSITION_Y]       = depth;
  target_eye_1->c[EYE_IRIS_POSITION_Y]  = depth;

  target_eye_2->c[EYE_POSITION_Y]       = -depth;
  target_eye_2->c[EYE_IRIS_POSITION_Y]  = -depth;

  Eye* target_eye[]      = {target_eye_1, target_eye_2, eye_copy(target_eye_1)};
  float go_durations[]   = {speed, 2*speed, 2*speed,speed};
  float stay_durations[] = {2*speed, 2*speed, 2*speed};

  eye_go_to_poses_stay_and_return(eye_global_eye, eye_global_queues, 3, target_eye, go_durations, stay_durations);
}
