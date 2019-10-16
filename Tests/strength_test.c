#include <stdio.h>
#include <math.h>

/*--------------------------------------------------------------------*/
float       rhythm_default_strength_for_onset(float beat_time, int n)
{
  int num_a=0, denom_a=1, num_b=1, denom_b=1, num_c, denom_c;
  float a,b,c;
  
  beat_time -= floor(beat_time);
  
  while((denom_a + denom_b) <= n)
    {
      num_c = num_a + num_b;
      denom_c = denom_a + denom_b;

      c = num_c / (float)denom_c;

      if(beat_time < c)
        {num_b=num_c; denom_b=denom_c;}
      else
        {num_a=num_c; denom_a=denom_c;}

      a = num_a / (float)denom_a;
      if(a == beat_time)
        break;
    }

  b = num_b / (float)denom_b;
  if(fabs(b-beat_time) < fabs(a-beat_time))  
     denom_a = denom_b;

  return 1.0 / denom_a;
}

/*--------------------------------------------------------------------*/
int main(void)
{
/*
  float t = 6 /7.0;
  int n = 5;
  float result = rhythm_default_strength_for_onset(t, n);
  fprintf(stderr, "\r\nbeat_time: %f\tstrength: %f\r\n", t, result);
*/


  int n = 8;
  int i, N=100;
  for(i=0; i<N; i++)
    {
      float result = rhythm_default_strength_for_onset(i / (float)N , n);
      fprintf(stderr, "%f\r\n", result);
    }
}
