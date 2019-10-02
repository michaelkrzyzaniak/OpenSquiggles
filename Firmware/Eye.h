#ifndef EYE
#define EYE 1

#ifdef __cplusplus
extern "C" {
#endif

void eye_init_module();

void eye_animate_blink            ();
void eye_animate_inquisitive      ();
void eye_animate_focused          ();
void eye_animate_suprised         ();
void eye_animate_neutral_size     ();
void eye_animate_neutral_position ();
//void eye_animate_single_param     (eye_param_t param, int val, float millisecs);
void eye_animate_roll             (int depth);
void eye_animate_shifty           (int depth, float speed);
void eye_animate_yes              (int depth, float speed);

#ifdef __cplusplus
}
#endif

#endif //EYE
