#ifndef __BodePlot__
#define __BodePlot__
  
#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

#include "MKAiff.h"

typedef struct opaque_bode_struct Bode;

Bode*  bode_new(MKAiff* aiff);
Bode*  bode_new_with_aiff_filename  (char* filename);
Bode*  bode_new_from_spectrum       (float* spectrum, int fft_n);
Bode*  bode_new_with_new_one_second_recording(); //1 channel
Bode*  bode_new_with_new_one_second_impedance_recording(); //2 channels
void   bode_init                    (Bode* self);
Bode*  bode_destroy                 (Bode* self);
void   bode_set_is_log_freq_scale   (Bode* self, int is_log);
int    bode_get_is_log_freq_scale   (Bode* self);
//void   bode_set_is_centered_at_0_db (Bode* self, int is_centered);
//int    bode_get_is_centered_at_0_db (Bode* self);
void   bode_center_at_0_db          (Bode* self, float min_freq, float max_freq);
void   bode_set_is_face_2_face      (Bode* self, int is_face);
int    bode_get_is_face_2_face      (Bode* self);
void   bode_set_is_unit_step        (Bode* self, int is_step);
int    bode_get_is_unit_step        (Bode* self);
void   bode_set_is_impedance        (Bode* self, int impedance);
int    bode_get_is_impedance        (Bode* self);
void   bode_set_line_width          (Bode* self, float width);
float  bode_get_line_width          (Bode* self);
void   bode_set_min_db              (Bode* self, float db);
float  bode_get_min_db              (Bode* self);
void   bode_set_max_db              (Bode* self, float db);
float  bode_get_max_db              (Bode* self);
void   bode_set_min_degrees         (Bode* self, float db);
float  bode_get_min_degrees         (Bode* self);
void   bode_set_max_degrees         (Bode* self, float db);
float  bode_get_max_degrees         (Bode* self);
void   bode_lifter                  (Bode* self, int magnitude, int phase, int num_cepstral_coeffs);
void   bode_make_image              (Bode* self, int magnitude, int phase); //does not first calculate the spectrum
void   bode_calculate_spectrum      (Bode* self); //does not draw a new image
void   bode_calculate_impedance     (Bode* self, int is_admittance); //does not draw a new image
void   bode_plot                    (Bode* self); //calls calculate_spectrum then make_image
void   bode_set_audio_if_response   (Bode* self, Bode* audio_if /*user must configure and calculate spectrum*/);
void   bode_save_image_as_png       (Bode* self, char* filename);
void   bode_save_audio_as_aiff      (Bode* self, char* filename);
int    bode_is_conformable_with     (Bode* self, Bode* a);
int    bode_average_together_spectra(Bode* self, Bode** spectra, int num_spectra); //averages together self and spectra (mag and pahse separately). Returns num things averaged together

void   bode_self_equals_self_minus_a(Bode* self, Bode* a); //spectra must be calculated first
void   bode_self_equals_self_plus_a (Bode* self, Bode* a); //spectra must be calculated first
void   bode_self_equals_self_plus_const (Bode* self, double a); //spectra must be calculated first
void   bode_self_equals_self_times_const(Bode* self, float c);

#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif   //__BodePlot__
