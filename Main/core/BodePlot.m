#include <Cocoa/Cocoa.h>
#include "BodePlot.h"
//#include "Microphone.h"
//#include "Synth.h"
//#include "Synth2.h"
#include "../../Beat-and-Tempo-Tracking/src/DFT.h"
//#include "Util.h"

#define BODE_IMAGE_WIDTH          2700
#define BODE_IMAGE_HEIGHT         900
#define BODE_IMAGE_PADDING        (BODE_IMAGE_WIDTH * 0.08)
#define BODE_PADDED_IMAGE_WIDTH   (BODE_IMAGE_WIDTH  + (2*BODE_IMAGE_PADDING))
#define BODE_PADDED_IMAGE_HEIGHT  (BODE_IMAGE_HEIGHT + (2*BODE_IMAGE_PADDING))

#define SIN_TWO_PI 0

#include <fenv.h>

typedef NSBitmapImageRep bode_image_t;

float scalef(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/*-----------------------------------------------------------------------*/
struct opaque_bode_struct
{
  MKAiff* aiff;
  int is_log_freq_scale;
  int is_centered_at_0_db;
  int is_face_2_face;
  int is_unit_step;
  int is_impedance; //2-channel

  int aiff_n;
  int fft_n;
  dft_sample_t* real;
  dft_sample_t* imag; 

  bode_image_t* image;
  float line_width;
  float max_db;
  float min_db;
  float max_degrees;
  float min_degrees;
  Bode* audio_if;
};


/*-----------------------------------------------------------------------*/
bode_image_t* bode_new_image(int width, int height)
{
  return[[NSBitmapImageRep alloc]
          initWithBitmapDataPlanes:NULL
          pixelsWide:width
          pixelsHigh:height
          bitsPerSample:8
          samplesPerPixel:4
          hasAlpha:YES
          isPlanar:NO
          colorSpaceName:NSDeviceRGBColorSpace
          bitmapFormat:NSAlphaFirstBitmapFormat
          bytesPerRow:0
          bitsPerPixel:0
        ];
}

/*-----------------------------------------------------------------------*/
Bode* bode_new(MKAiff* aiff)
{
  Bode* self = calloc(1, sizeof(*self));
  if(self != NULL)
    {
      self->aiff = aiff;
      if(self->aiff == NULL) return bode_destroy(self);
    
      int num_channels = aiffNumChannels(self->aiff);

      self->aiff_n = aiffDurationInFrames(self->aiff);
      self->fft_n  = dft_smallest_power_of_2_at_least_as_great_as(self->aiff_n);

      self->real = calloc(self->fft_n * num_channels, sizeof(*self->real));
      if(self->real == NULL) return bode_destroy(self);
      
      self->imag = calloc(self->fft_n * num_channels, sizeof(*self->imag));
      if(self->imag == NULL) return bode_destroy(self);

      self->image = bode_new_image(BODE_PADDED_IMAGE_WIDTH, BODE_PADDED_IMAGE_HEIGHT);
      if(self->image == NULL) return bode_destroy(self);
    
      bode_init(self);
    }
  return self;
}

/*-----------------------------------------------------------------------*/
Bode* bode_new_from_spectrum(float* spectrum, int fft_n)
{
  Bode* self = calloc(1, sizeof(*self));
  if(self != NULL)
    {
      self->fft_n  = fft_n;

      self->real = calloc(self->fft_n, sizeof(*self->real));
      if(self->real == NULL) return bode_destroy(self);
      
      self->imag = calloc(self->fft_n, sizeof(*self->imag));
      if(self->imag == NULL) return bode_destroy(self);

      self->image = bode_new_image(BODE_PADDED_IMAGE_WIDTH, BODE_PADDED_IMAGE_HEIGHT);
      if(self->image == NULL) return bode_destroy(self);
    
      int i;
      for(i=0; i<fft_n; i++)
        self->real[i] = spectrum[i];
    
      bode_init(self);
    }
  return self;
}


/*-----------------------------------------------------------------------*/
Bode* bode_new_with_aiff_filename(char* filename)
{
  MKAiff* aiff = aiffWithContentsOfFile(filename);
  return bode_new(aiff);
}

/*-----------------------------------------------------------------------*/
Bode* bode_new_with_new_one_second_recording_private(int num_channels)
{

}

/*-----------------------------------------------------------------------*/
Bode* bode_new_with_new_one_second_recording()
{
  return bode_new_with_new_one_second_recording_private(1);
}

/*-----------------------------------------------------------------------*/
Bode* bode_new_with_new_one_second_impedance_recording()
{
  return bode_new_with_new_one_second_recording_private(2);
}

/*-----------------------------------------------------------------------*/
void bode_init(Bode* self)
{
  self->is_unit_step        = false;
  self->is_log_freq_scale   = false;
  self->is_impedance        = 0;//aiffNumChannels(self->aiff) == 2;
  self->is_centered_at_0_db = true;
  self->is_face_2_face      = false;
  self->line_width          = 1.0;
  self->min_db              = -10;
  self->max_db              = 10;
  self->min_degrees         = -180;
  self->max_degrees         = 180;
}

/*-----------------------------------------------------------------------*/
Bode* bode_destroy(Bode* self)
{
  if(self != NULL)
    {
      if(self->real != NULL)
        free(self->real);
      if(self->imag != NULL)
        free(self->imag);
      if(self->image != NULL)
        [self->image release];
      free(self);
    }
  return (Bode*) NULL;
}

/*-----------------------------------------------------------------------*/
void bode_set_is_log_freq_scale(Bode* self, int is_log)
{
  self->is_log_freq_scale = is_log != 0;
}

/*-----------------------------------------------------------------------*/
int bode_get_is_log_freq_scale(Bode* self)
{
  return self->is_log_freq_scale;
}

/*-----------------------------------------------------------------------*/
/*
void bode_set_is_centered_at_0_db(Bode* self, int is_centered)
{
  self->is_centered_at_0_db = is_centered != 0;
}
*/
/*-----------------------------------------------------------------------*/
/*
int bode_get_is_centered_at_0_db(Bode* self)
{
  return self->is_centered_at_0_db;
}
*/
/*-----------------------------------------------------------------------*/
void bode_set_is_face_2_face(Bode* self, int is_face)
{
  self->is_face_2_face = is_face != 0;
}

/*-----------------------------------------------------------------------*/
int bode_get_is_face_2_face(Bode* self)
{
  return self->is_face_2_face;
}

/*-----------------------------------------------------------------------*/
void bode_set_is_unit_step(Bode* self, int is_step)
{
  self->is_unit_step = is_step != 0;
}

/*-----------------------------------------------------------------------*/
int bode_get_is_unit_step(Bode* self)
{
  return self->is_unit_step;
}

/*-----------------------------------------------------------------------*/
void   bode_set_is_impedance        (Bode* self, int impedance)
{
  self->is_impedance = impedance;
}

/*-----------------------------------------------------------------------*/
int    bode_get_is_impedance        (Bode* self)
{
  return self->is_impedance;
}

/*-----------------------------------------------------------------------*/
void bode_set_line_width(Bode* self, float width)
{
  if(width < 1 ) width = 1;
  if(width > 10) width = 10;
  self->line_width = width;
}

/*-----------------------------------------------------------------------*/
float bode_get_line_width(Bode* self)
{
  return self->line_width;
}

/*-----------------------------------------------------------------------*/
void bode_set_min_db(Bode* self, float db)
{
  //if(db > -10 ) db = -10;
  self->min_db = db;
}

/*-----------------------------------------------------------------------*/
float bode_get_min_db(Bode* self)
{
  return self->min_db;
}

/*-----------------------------------------------------------------------*/
void bode_set_max_db(Bode* self, float db)
{
  //if(db < 10 ) db = 10;
  self->max_db = db;
}

/*-----------------------------------------------------------------------*/
float bode_get_max_db(Bode* self)
{
  return self->max_db;
}

/*-----------------------------------------------------------------------*/
void   bode_set_min_degrees         (Bode* self, float degrees)
{
  self->min_degrees = degrees;
}

/*-----------------------------------------------------------------------*/
float  bode_get_min_degrees         (Bode* self)
{
  return self->min_degrees;
}

/*-----------------------------------------------------------------------*/
void   bode_set_max_degrees         (Bode* self, float degrees)
{
  self->max_degrees = degrees;
}

/*-----------------------------------------------------------------------*/
float  bode_get_max_degrees         (Bode* self)
{
  return self->max_degrees;
}

/*-----------------------------------------------------------------------*/
int bode_is_conformable_with(Bode* self, Bode* a)
{
  return (self->fft_n == a->fft_n) && (aiffSampleRate(self->aiff) == aiffSampleRate(a->aiff));
}

//todo: enforce that spectra are calculated first!!!!!
/*-----------------------------------------------------------------------*/
void bode_self_equals_self_minus_a(Bode* self, Bode* a)
{
  int i;
  if(bode_is_conformable_with(self, a))
    for(i=0; i<self->fft_n; i++)
      self->real[i] -= a->real[i];
}

//todo: enforce that spectra are calculated first!!!!!
/*-----------------------------------------------------------------------*/
void bode_self_equals_self_plus_a(Bode* self, Bode* a)
{
  int i;
  if(bode_is_conformable_with(self, a))
    for(i=0; i<self->fft_n; i++)
      self->real[i] += a->real[i];
}

//todo: enforce that spectra are calculated first!!!!!
/*-----------------------------------------------------------------------*/
void bode_self_equals_self_plus_const(Bode* self, double a)
{
  int i;
  for(i=0; i<self->fft_n; i++)
    self->real[i] += a;
}

//todo: enforce that spectra are calculated first!!!!!
/*-----------------------------------------------------------------------*/
void bode_self_equals_self_times_const(Bode* self, float c)
{
  int i;
    for(i=0; i<self->fft_n; i++)
      self->real[i] *= c;
}

/*-----------------------------------------------------------------------*/
void bode_calculate_impedance(Bode* self, int is_admittance)
{
  double SMALL = 1E-20;
  double BRIDGE_RESISTOR = 2173;
  //double BRIDGE_RESISTOR = 46300;
  //double BRIDGE_RESISTOR = 9900;
  //double BRIDGE_RESISTOR = 983;
  //double BRIDGE_RESISTOR = 98.6;
  
  int i;
  int num_channels = aiffNumChannels(self->aiff);

  if(num_channels != 2)
    {fprintf(stderr, "cannot calculate impedance. Must be a 2-channel recording.\r\n"); return;}

  bode_calculate_spectrum(self);

  for(i=1; i<self->fft_n; i++)
    {
      self->real[i] = self->real[i + self->fft_n] - self->real[i];
      self->imag[i] = self->imag[i + self->fft_n] - self->imag[i] + (M_PI * 2);
      while(self->imag[i] >  M_PI)  self->imag[i] -= 2*M_PI;
      while(self->imag[i] <= -M_PI) self->imag[i] += 2*M_PI;

      int sign = (self->imag[i] >= 0) ? 1 : -1;
      double voltage_ratio = pow(10, self->real[i] / 20.0);
      double cos_theta = cos(self->imag[i]);
      double temp = 1.0 + (voltage_ratio * voltage_ratio) - (2.0 * voltage_ratio * cos_theta);
      if(temp < SMALL) temp = SMALL;
      temp = sqrt(temp);
      double impedance_magnitude = BRIDGE_RESISTOR * voltage_ratio / temp;
      double admittance_magnitude = temp / (BRIDGE_RESISTOR * voltage_ratio);
      double impedance_rseries = BRIDGE_RESISTOR * (voltage_ratio - cos_theta) / ((2.0 * cos_theta) - voltage_ratio - (1.0 / voltage_ratio));
      double temp2 = impedance_magnitude*impedance_magnitude - impedance_rseries*impedance_rseries;
      if(temp2 < 0.0) temp2 = 0.0;
      double impedance_xseries = sign * sqrt(temp2);
      double impedance_angle = atan2(impedance_xseries, impedance_rseries);
      while(impedance_angle > M_PI/2) impedance_angle -= 2*M_PI;
      double admittance_angle = -impedance_angle;
  
      if(is_admittance)
        {
          self->real[i] = 20 * log10(admittance_magnitude);
          self->imag[i] = admittance_angle;
        }
      else
        {
          self->real[i] = impedance_magnitude;
          self->imag[i] = impedance_angle;
        }

    /*
//FREQ_RESPONSE
      double INPUT_RESISTANCE = 1000000;
      double freq_response_real = impedance_rseries + INPUT_RESISTANCE;
      double freq_response_imag = impedance_xseries;
      double freq_response_mag  = hypot(freq_response_real, freq_response_imag);
      double freq_response_angle= atan2(freq_response_imag, freq_response_real);
      freq_response_mag = INPUT_IRESISTANCE / freq_response_mag;
      freq_response_angle= 0 - freq_response_angle;
    
      self->real[i] = 20*log10(freq_response_mag);
      self->imag[i] = freq_response_angle;
  */
    }
//LAZYNESS
    //bode_set_min_db              (self, -70);
    //bode_set_max_db              (self,  10);
    //bode_set_line_width          (self,  5);
    //bode_self_equals_self_plus_const (self, 50);
    //bode_center_at_0_db(self, 1000, 2000);
}

/*-----------------------------------------------------------------------*/
void bode_center_at_0_db(Bode* self, float min_freq, float max_freq)
{
  //float min_freq     = 1000;
  //float max_freq = 2000;//192000;
  //float min_freq     = 19000;
  //float max_freq = 22000;//192000;
  int bin_of_min_freq;
  int bin_of_max_freq;
  float sample_rate = aiffSampleRate(self->aiff);
  
  for(bin_of_min_freq=1; bin_of_min_freq<self->fft_n / 2; bin_of_min_freq++)
    if(dft_frequency_of_bin(bin_of_min_freq, sample_rate, self->fft_n) > min_freq)
      break;

  for(bin_of_max_freq=bin_of_min_freq; bin_of_max_freq<self->fft_n / 2; bin_of_max_freq++)
    if(dft_frequency_of_bin(bin_of_max_freq, sample_rate, self->fft_n) >= max_freq)
      break;

  int i;
  double average = 0;
  for(i=bin_of_min_freq; i<bin_of_max_freq; i++)
    average += self->real[i];
  average /= bin_of_max_freq - bin_of_min_freq;
  for(i=1; i<self->fft_n / 2; i++)
    self->real[i] -= average;
  
  fprintf(stderr, "average: %lf\r\n", average);
}

/*-----------------------------------------------------------------------*/
int    bode_average_together_spectra(Bode* self, Bode** spectra, int num_spectra)
{
  int i, j, N=1;

  //average impedances
  for(i=0; i<num_spectra; i++)
    {
      if(bode_is_conformable_with(self, spectra[i]))
        {
          ++N;
          for(j=0; j<self->fft_n; j++)
            {
              self->real[j] = self->real[j] + (spectra[i]->real[j] - self->real[j]) / (double)N;
              self->imag[j] = self->imag[j] + (spectra[i]->imag[j] - self->imag[j]) / (double)N;
            }
        }
    }
    return N;
}

/*-----------------------------------------------------------------------*/
/*
  Reciprocity calibration method for ultrasonic piezoelectric transducers in air.
  Comparison of finite element modelling and experimental measurements
  Andersen, K. K.∗a, Søvik, A. A.a, Lunde, P.a,b,c,
  Vestrheim, M.a,c, and Kocbach, J.b,c
  Proceedings of the 38th Scandinavian Symposium on Physical Acoustics
  1 - 4 February 2015
*/
int bode_bergen_calibration(Bode* self, Bode* Z3, Bode* H1, Bode* H2, Bode* H3, double d1, double d2, double d3)
{
  int i;
  
  if(!bode_is_conformable_with(self, Z3)) return 0;
  if(!bode_is_conformable_with(self, H1)) return 0;
  if(!bode_is_conformable_with(self, H2)) return 0;
  if(!bode_is_conformable_with(self, H3)) return 0;
  
  double Jr, Ji;
  double density_of_air =  1.225; // kg/m3, 15ºC Sea Level
  double mag, phase;
  double d0 = 1; //meter

  for(i=1; i<self->fft_n; i++)
    {
      //equation 5
      Jr = (2*d0) / (density_of_air * dft_frequency_of_bin(i, aiffSampleRate(self->aiff), self->fft_n));
      Ji = -M_PI + (i*d0);
    
      //equation 7
      //if(H2->real[i] != 0) //just let there be a divide by 0 error for now
      mag   = Jr * Z3->real[i] * H1->real[i] * H3->real[i] / H2->real[i];
      phase = Ji + Z3->imag[i] + H1->imag[i] + H3->imag[i] - H2->imag[i];
    
      mag   *= (d1/d0)*(d3/d2);
      phase += i*(d1+d3-d0-d2);
    
      mag = sqrt(mag);
      phase *= 0.5;
    
      self->real[i] = mag;
      self->imag[i] = phase;
    }
  return 1;
}

/*-----------------------------------------------------------------------*/
void bode_calculate_spectrum(Bode* self)
{

}

/*-----------------------------------------------------------------------*/
void bode_lifter(Bode* self, int magnitude, int phase, int num_cepstral_coeffs)
{
  if(num_cepstral_coeffs >= self->fft_n >> 1) return;
  
  int i, chan;
  int num_channels = aiffNumChannels(self->aiff);
  
  dft_sample_t* scratch = malloc(self->fft_n * sizeof(*scratch));
  if(scratch == NULL) {perror("error while liftering"); return;}
  
  for(chan=0; chan<num_channels; chan++)
    {
      if(magnitude)
        {
          //this helps with numerical stability
          self->real[chan*self->fft_n] = 0;
          self->real[self->fft_n / 2 + chan*self->fft_n] = 0;
        
          memset(scratch, 0, self->fft_n * sizeof(*scratch));
          dft_real_inverse_dft(self->real + chan*self->fft_n, scratch, self->fft_n);
          for(i=num_cepstral_coeffs; i<(self->fft_n >> 1); i++)
            {
              self->real[i + chan*self->fft_n] = 0;
              scratch[i] = 0;
              self->real[self->fft_n - i - 1 + chan*self->fft_n] = 0;
              scratch[self->fft_n - i - 1] = 0;
            }
          dft_real_forward_dft(self->real + chan*self->fft_n, scratch, self->fft_n);
        }

      if(phase)
        {
          //this helps with numerical stability
          self->imag[chan*self->fft_n] = 0;
          self->imag[self->fft_n / 2 + chan*self->fft_n] = 0;
        
          memset(scratch, 0, self->fft_n * sizeof(*scratch));
          dft_real_inverse_dft(self->imag + chan*self->fft_n, scratch, self->fft_n);
          for(i=num_cepstral_coeffs; i<(self->fft_n >> 1); i++)
            {
              self->imag[i + chan*self->fft_n] = 0;
              scratch[i] = 0;
              self->imag[self->fft_n - i - 1 + chan*self->fft_n] = 0;
              scratch[self->fft_n - i - 1] = 0;
            }
          dft_real_forward_dft(self->imag + chan*self->fft_n, scratch, self->fft_n);
        }
    }
  
  free(scratch);
}

/*-----------------------------------------------------------------------*/
void bode_save_audio_as_aiff(Bode* self, char* filename)
{
  aiffSaveWithFilename(self->aiff, filename);

}

/*-----------------------------------------------------------------------*/
void bode_save_image_as_png(Bode* self, char* filename)
{
  NSMutableString* ns_filename = [[NSMutableString alloc] initWithCString: filename encoding: NSASCIIStringEncoding];
  //[filename appendString:@".png"];
  NSData* imageData = [self->image representationUsingType: NSPNGFileType properties:nil];
  [imageData writeToFile: ns_filename atomically: NO];
}

/*-----------------------------------------------------------------------*/
void bode_make_image(Bode* self, int magnitude, int phase)
{
  int width         = BODE_IMAGE_WIDTH;
  int height        = BODE_IMAGE_HEIGHT;
  int padding       = BODE_IMAGE_PADDING;
  int padded_width  = BODE_PADDED_IMAGE_WIDTH;
  int padded_height = BODE_PADDED_IMAGE_HEIGHT;
  int i, j;
  float sample_rate = 44100;//aiffSampleRate(self->aiff);
  int N = (self->fft_n / 2) - 1;

  NSMutableDictionary* fontAttributes = [NSMutableDictionary dictionary];
  NSMutableParagraphStyle* style = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
  style.alignment = NSTextAlignmentCenter;
  [fontAttributes setObject:[NSFont fontWithName:@"Helvetica Bold" size:44] forKey:NSFontAttributeName];
  [fontAttributes setObject:style forKey:NSParagraphStyleAttributeName];

  float min_phase = self->min_degrees;
  float max_phase = self->max_degrees;

  float min_freq     = 10;
  float log_min_freq = log10(min_freq);
  //float max_freq     = dft_frequency_of_bin(N, sample_rate, 2*(N+1));
  float max_freq = 22050;//192000;
  float log_max_freq = log10(max_freq);
  int bin_of_min_freq;
  int bin_of_max_freq;

  for(bin_of_min_freq=1; bin_of_min_freq<N; bin_of_min_freq++)
    if(dft_frequency_of_bin(bin_of_min_freq, sample_rate, 2*(N+1)) > min_freq)
      break;

  for(bin_of_max_freq=bin_of_min_freq; bin_of_max_freq<2*N+1; bin_of_max_freq++)
    if(dft_frequency_of_bin(bin_of_max_freq, sample_rate, 2*(N+1)) >= max_freq)
      break;

  float max_db = self->max_db;
  float min_db = self->min_db;
/*
  for(i=bin_of_min_freq; i<N; i++)
    {
      if(self->real[i] > max_db)
        max_db = self->real[i];
      if(self->real[i] < min_db)
        min_db = self->real[i];
    }
*/
  //max_db = 10 * ceil(max_db / 10);
  //min_db = 10 * floor(min_db / 10);

  [NSGraphicsContext saveGraphicsState];
  [NSGraphicsContext setCurrentContext:[NSGraphicsContext graphicsContextWithBitmapImageRep:self->image]];
  
  NSBezierPath *path = [[NSBezierPath alloc] init];
  NSPoint p = {0, scalef(self->real[0], 0, min_db, height, 0)};
  NSRect rect;
  
  //clear background
  rect.origin.x    = 0;
  rect.origin.y    = 0;
  rect.size.width  = padded_width;
  rect.size.height = padded_height;
  [path appendBezierPathWithRect:rect];
  [[NSColor colorWithCalibratedHue:0.0 saturation: 0.0 brightness: 1 alpha: 1] setFill];
  [path fill];
  [path removeAllPoints];
  
  //draw decibel lines 
  [[NSColor blackColor] setStroke];
  for(i=max_db; i>=min_db; i -= (max_db - min_db) / 8.0)
    {
      p.x = padding / 6;
      p.y = padding + scalef(i, max_db, min_db, height, 0) - 22;
      [[NSString stringWithFormat:@"   %d", i] drawAtPoint:p withAttributes:fontAttributes];
      p.y += 22;
      p.x = padding;
      [path moveToPoint: p];
      p.x = padding + width;
      [path lineToPoint: p];
      [path stroke];
      [path removeAllPoints];
    }

  if(phase)
    {
      [fontAttributes setObject:[NSColor lightGrayColor] forKey:NSForegroundColorAttributeName];
      for(i=max_phase; i>=min_phase; i -= (max_phase - min_phase) / 8.0)
        {
          p.x = width + padding + padding / 6;
          p.y = padding + scalef(i, max_phase, min_phase, height, 0) - 22;
          [[NSString stringWithFormat:@"%dº", i] drawAtPoint:p withAttributes:fontAttributes];
         }
        [fontAttributes setObject:[NSColor blackColor] forKey:NSForegroundColorAttributeName];
    }
  
  //draw log frequency lines 
  if(self->is_log_freq_scale)
  {
    for(i=log_min_freq; i<=log_max_freq; i++)
      {
        for(j=1; j<10; j++)
          {
            float log_freq = log10(pow(10, i) * j);
            if(log_freq >= log_max_freq) break;
          
            p.x = scalef(log_freq, log_min_freq, log_max_freq, padding, padding+width) - 22;
      
            if(j==1)
              {
                p.y = 2 * padding / 3;
                [[NSString stringWithFormat:@"%d Hz", (int)(pow(10, log_freq))] drawAtPoint:p withAttributes:fontAttributes];
                [[NSColor blackColor] setStroke];
              }
            else
              [[NSColor lightGrayColor] setStroke];
       
            p.x += 22;
            p.y = padding;
            [path moveToPoint: p];
            p.y = padding + height;
            [path lineToPoint: p];
            [path stroke];
            [path removeAllPoints];
          }
      }
    }

  //draw linear frequency lines
  else
    {
      for(i=min_freq; i<max_freq; i+= 1000)
        {
          int freq = i;
          if(i == min_freq)
            {
              p.x = scalef(freq, min_freq, max_freq, padding, padding+width) - 22;
              p.y = 2 * padding / 3;
              [[NSString stringWithFormat:@"%d", i] drawAtPoint:p withAttributes:fontAttributes];           
            }
          else
            {
              freq = floor(i * 0.001);
              p.x = scalef(1000 * freq, min_freq, max_freq, padding, padding+width) - 22;
              p.y = 2 * padding / 3;
              [[NSString stringWithFormat:@"%dk", freq] drawAtPoint:p withAttributes:fontAttributes];
            }
          p.x += 22;
          p.y = padding;
          [path moveToPoint: p];
          p.y = padding + height;
          [path lineToPoint: p];
          [path stroke];
          [path removeAllPoints];
        }
    }

  [path setLineWidth:self->line_width];
  //draw phase plot
  if(phase)
    {
      [[NSColor lightGrayColor] setStroke];
      if(self->imag[bin_of_min_freq] > M_PI) self->imag[bin_of_min_freq] = M_PI;
      if(self->imag[bin_of_min_freq] < -M_PI) self->imag[bin_of_min_freq] = -M_PI;
      p.x = (self->is_log_freq_scale) ?
        padding + scalef(log10(dft_frequency_of_bin(bin_of_min_freq, sample_rate, 2*(N+1))), log_min_freq, log_max_freq, 0, width) :
        padding + scalef(dft_frequency_of_bin(bin_of_min_freq, sample_rate, 2*(N+1)), min_freq, max_freq, 0, width);
      p.y = padding + scalef(180.0 / M_PI * self->imag[bin_of_min_freq], max_phase, min_phase, height, 0);
      [path moveToPoint: p];
      for(i=bin_of_min_freq+1; i<bin_of_max_freq; i++)
        {
          if(self->imag[i] > M_PI) self->imag[i] = M_PI;
          if(self->imag[i] < -M_PI) self->imag[i] = -M_PI;
          p.x = (self->is_log_freq_scale) ?
            padding + scalef(log10(dft_frequency_of_bin(i, sample_rate, 2*(N+1))), log_min_freq, log_max_freq, 0, width) :
            padding + scalef(dft_frequency_of_bin(i, sample_rate, 2*(N+1)), min_freq, max_freq, 0, width);
          p.y = padding + scalef(180.0 / M_PI * self->imag[i], max_phase, min_phase, height, 0);
          [path lineToPoint: p];
        }
      [path stroke];
      [path removeAllPoints];
    }
  
 //draw magnitude plot
  if(magnitude)
    {
      [[NSColor blackColor] setStroke];
      if(self->real[bin_of_min_freq] > max_db) self->real[bin_of_min_freq] = max_db;
      if(self->real[bin_of_min_freq] < min_db) self->real[bin_of_min_freq] = min_db;
      p.x = (self->is_log_freq_scale) ?
        padding + scalef(log10(dft_frequency_of_bin(bin_of_min_freq, sample_rate, 2*(N+1))), log_min_freq, log_max_freq, 0, width) :
        padding + scalef(dft_frequency_of_bin(bin_of_min_freq, sample_rate, 2*(N+1)), min_freq, max_freq, 0, width);
      p.y = padding + scalef(self->real[bin_of_min_freq], max_db, min_db, height, 0);
      [path moveToPoint: p];
      for(i=bin_of_min_freq+1; i<bin_of_max_freq; i++)
        {
          if(self->real[i] > max_db) self->real[i] = max_db;
          if(self->real[i] < min_db) self->real[i] = min_db;
          p.x = (self->is_log_freq_scale) ?
            padding + scalef(log10(dft_frequency_of_bin(i, sample_rate, 2*(N+1))), log_min_freq, log_max_freq, 0, width) :
            padding + scalef(dft_frequency_of_bin(i, sample_rate, 2*(N+1)), min_freq, max_freq, 0, width);
          p.y = padding + scalef(self->real[i], max_db, min_db, height, 0);
          [path lineToPoint: p];
        }
      [path stroke];
      [path removeAllPoints];
    }
  
  rect.origin.x    = padding;
  rect.origin.y    = padding;
  rect.size.width  = width;
  rect.size.height = height;
  [path appendBezierPathWithRect:rect];
  [path setLineWidth:5.0];
  [[NSColor blackColor] setStroke];
  [path stroke];  
  [path removeAllPoints];
  
  //draw top labels
  double square_size = width * 0.0175;
  rect.size.width  = rect.size.height = square_size;
  rect.origin.x    = padding;//width * 0.5;
  rect.origin.y    = height + (padding * 1.5) + (0.5 * square_size) + (0.25 * square_size);
  rect.origin.y   -= (1.5 * square_size);
  [path appendBezierPathWithRect:rect];
  [[NSColor blackColor] setStroke];
  [[NSColor blackColor] setFill];
  [path stroke];
  [path fill];
  [path removeAllPoints];
  p.x = rect.origin.x + 1.5*square_size;
  p.y = rect.origin.y;
  [[NSString stringWithFormat:@"Magnitude"] drawAtPoint:p withAttributes:fontAttributes];
  
  if(phase){
  rect.origin.y   += (1.5 * square_size);
  [path appendBezierPathWithRect:rect];
  [[NSColor blackColor] setStroke];
  [[NSColor lightGrayColor] setFill];
  [path stroke];
  [path fill];
  [path removeAllPoints];
  p.x = rect.origin.x + 1.5*square_size;
  p.y = rect.origin.y;
  [[NSString stringWithFormat:@"Phase"] drawAtPoint:p withAttributes:fontAttributes];
  }
  
  p.x = width + padding - 650;
  p.y = height + (padding * 1.1);
  [[NSString stringWithFormat:@"%d-point DFT at %d Hz", 2*(N+1), (int)sample_rate] drawAtPoint:p withAttributes:fontAttributes];
  
  [NSGraphicsContext restoreGraphicsState];
}

/*-----------------------------------------------------------------------*/
void bode_set_audio_if_response   (Bode* self, Bode* audio_if)
{
  self->audio_if = audio_if;
}

/*-----------------------------------------------------------------------*/
void bode_plot(Bode* self)
{
  bode_calculate_spectrum(self);
  bode_make_image(self, YES, YES);
}

