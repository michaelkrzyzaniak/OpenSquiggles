void solenoid_init           ();
void solenoid_tap            (float strength);
void solenoid_ding           (float strength);
void solenoid_tap_specific   (int index, float strength);
void solenoid_on             (int index);
void solenoid_on_for_duration(int index, float duration);
void solenoid_off            (int index);
void solenoid_all_off       ();
