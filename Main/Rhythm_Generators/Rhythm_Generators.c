#include "Rhythm_Generators.h"
#include <stdlib.h> //calloc
#include <math.h>


/*--------------------------------------------------------------------*/
void rhythm_get_rational_approximation(float onset_time, int n, int* num, int* denom)
{
  int num_a=0, denom_a=1, num_b=1, denom_b=1, num_c, denom_c, extra;
  float a=0,b,c;
  
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
}

/*--------------------------------------------------------------------*/
float rhythm_get_default_onset_strength(float onset_time, int n)
{
  int num, denom;
  
  rhythm_get_rational_approximation(onset_time, n, &num, &denom);
  
  return 1.0 / denom;
}
