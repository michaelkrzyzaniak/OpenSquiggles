#include "Rhythm_Generators.h"
#include <stdlib.h> //calloc
#include <math.h>
#include <string.h> //memcpy


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

/*-------------------------------------------------------------------------------------*/
void rhythm_quicksort_swap_entry_content_private(rhythm_onset_t* onset_a, rhythm_onset_t* onset_b)
{
  rhythm_onset_t temp;
  size_t n = sizeof(temp);
  memcpy(&temp, onset_a, n);
  memcpy(onset_a, onset_b, n);
  memcpy(onset_b, &temp, n);
}

/*-------------------------------------------------------------------------------------*/
unsigned rhythm_quicksort_partition_private(rhythm_onset_t* rhythm, unsigned low_index, unsigned high_index)
{
  unsigned i, result=low_index;
  unsigned pivot_index = low_index + ((high_index - low_index) / 2);
  
  rhythm_onset_t* pivot_entry   = rhythm + pivot_index;
  rhythm_onset_t* high_entry    = rhythm + high_index;
  rhythm_onset_t* store_entry   = rhythm + low_index;
  rhythm_onset_t* current_entry = store_entry;
  
  rhythm_quicksort_swap_entry_content_private(pivot_entry, high_entry);
  
  for(i=low_index; i<high_index; i++)
    {
      if(high_entry->beat_time > current_entry->beat_time)
        {
          rhythm_quicksort_swap_entry_content_private(current_entry, store_entry);
          ++store_entry;
          ++result;
        }
      ++current_entry;
    }
  
  rhythm_quicksort_swap_entry_content_private(store_entry, high_entry);
  
  return result;
}

/*-------------------------------------------------------------------------------------*/
void rhythm_quicksort_private(rhythm_onset_t* rhythm, unsigned low_index, unsigned high_index)
{
  if(low_index < high_index)
    {
      unsigned p = rhythm_quicksort_partition_private(rhythm, low_index, high_index);
      if(p > 0) rhythm_quicksort_private(rhythm, low_index, p-1);
      rhythm_quicksort_private(rhythm, p+1, high_index);
    }
}

/*--------------------------------------------------------------------*/
void rhythm_sort_by_onset_time(rhythm_onset_t* rhythm, int n)
{
  if(n < 2) return;
  rhythm_quicksort_private(rhythm, 0, n-1);
}

