#include <stdio.h>
#include <math.h>

/*--------------------------------------------------------------------*/
float       rhythm_default_strength_for_onset(float beat_time, int n)
{
  //returns the 1 over the denominator of the nearest rational
  //number to beat_time, considering only denominators not
  //greater than n. Uses some damn thing related to the Ford
  //circle packing. Good luck trying to understand how it works.
  
  int i;
  int num_a=0, denom_a=1, num_b=1, denom_b=1, num_c, denom_c;
  float a,c;
  
  for(i=0; i<n; i++)
    {
      //fprintf(stderr, "a=%i/%i ... ", num_a, denom_a);
      //fprintf(stderr, "b=%i/%i ... ", num_b, denom_b);

      a = num_a / (float)denom_a;
      if(a == beat_time)
        break;
      
      if((denom_a + denom_b) > n)
        break;

      num_c = num_a + num_b;
      denom_c = denom_a + denom_b;

      c = num_c / (float)denom_c;
  
      //fprintf(stderr, "c=%i/%i\r\n", num_c, denom_c);

      if(beat_time < c)
        {num_b=num_c; denom_b=denom_c;}
      else
        {num_a=num_c; denom_a=denom_c;}
    }

  if(fabs(c-beat_time) < fabs(a-beat_time))  
     denom_a = denom_c;
  float b = num_b / (float)denom_b;
  if(fabs(b-beat_time) < fabs(a-beat_time))  
     denom_a = denom_b;

  return sqrt(1.0 / (float)denom_a);
}


/*--------------------------------------------------------------------*/
int main(void)
{
/*
  float t = 0.33;
  int n = 4;
  float result = rhythm_default_strength_for_onset(t, n);
  fprintf(stderr, "\r\nbeat_time: %f\tstrength: %f\r\n", t, result);

*/

  int n = 6;
  int i, N=100;
  for(i=0; i<N; i++)
    {
      float result = rhythm_default_strength_for_onset(i / (float)N , n);
      fprintf(stderr, "%f %f\r\n", i / (float)N, result);
    }

}