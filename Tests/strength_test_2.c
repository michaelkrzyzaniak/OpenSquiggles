#include <stdio.h>
#include <math.h>

/*--------------------------------------------------------------------*/
float rhythm_default_strength_for_onset(float onset_time, int n, int* num, int* denom)
{
  int num_a=0, denom_a=1, num_b=1, denom_b=1, num_c, denom_c, extra;
  float a,b,c;
  
  extra = floor(onset_time);
  onset_time -= extra;
  
  while((denom_a + denom_b) <= n)
    {
      num_c = num_a + num_b;
      denom_c = denom_a + denom_b;
      c = num_c / (float)denom_c;

      if(onset_time < c)
        {num_b=num_c; denom_b=denom_c;}
      else
        {num_a=num_c; denom_a=denom_c;}

      a = num_a / (float)denom_a;
      if(a == onset_time)
        break;
    }

  b = num_b / (float)denom_b;
  if(fabs(b-onset_time) < fabs(a-onset_time))
    {
      num_a = num_b;
      denom_a = denom_b;
    }
  *num = num_a + extra * denom_a;
  *denom = denom_a;
  
  return 1.0 / denom_a;
}


/*--------------------------------------------------------------------*/
int main(void)
{
  float t = -1.66;
  int n = 8;
  int num, denom;
  float result = rhythm_default_strength_for_onset(t, n, &num, &denom);
  fprintf(stderr, "beat_time: %f\tnum: %i denom: %i\r\n", t, num, denom);

/*
  int n = 6;
  int i, N=100;
  for(i=0; i<N; i++)
    {
      float result = rhythm_default_strength_for_onset(i / (float)N , n);
      fprintf(stderr, "%f %f\r\n", i / (float)N, result);
    }
*/
}
