#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define RHYTHM_LENGTH 16
#define ROW_LENGTH 22
#define ROWS_PER_TABLE 80

/*--------------------------------------------------------------------*/
float f(float x, float exponent, int is_inverse)
{

  is_inverse = is_inverse ? -1 : 1;
 
  float nonliniarity = fabs(2*x-1);
  nonliniarity = pow(nonliniarity, exponent);
  nonliniarity *= is_inverse;
  if(x < 0.5) nonliniarity *= -1;
  nonliniarity += 1;
  nonliniarity /= 2.0;
  return nonliniarity;
}

/*--------------------------------------------------------------------*/
float        rhythm_histogram_get_convergence_score(float* histogram, float exponent, int is_inverse)
{
  float result;
  
  float minimax = 2;
  float maximin = -1;
  int length = RHYTHM_LENGTH;
  int i;
  for(i=0; i<length; i++)
    {
      if(histogram[i] >= 0.5)
        {
          if(histogram[i] < minimax)
            minimax = histogram[i];
        }
      else
        {
          if(histogram[i] > maximin)
            maximin = histogram[i];
        }
    }
  
  if((minimax == 2) || (maximin == -1))
    result = 0;

  else
    //result = minimax - maximin;
    result = fabs(f(minimax, exponent, is_inverse) - f(maximin, exponent, is_inverse));

  return result;
}

/*--------------------------------------------------------------------*/
int read_table(FILE* file)
{
  float d;
  float exponent;
  float is_inverse;
  float row[ROW_LENGTH] = {-1};
  int i, j;

  if(3 != fscanf(file, "starting trial with d: %f k: %f i: %f", &d, &exponent, &is_inverse))
    return 0;  

  fprintf(stderr, "d: %f k: %f i: %f\r\n", d, exponent, is_inverse);  

  for(i=0; i<ROWS_PER_TABLE; i++)
    {
      for(j=0; j<ROW_LENGTH; j++)
        if(1 != fscanf(file, "%f", row+j))
          return 0;
      //histogram valse start at begining of for, the rest are ignored
      float conv = rhythm_histogram_get_convergence_score(row, exponent, is_inverse);
      fprintf(stderr, "%f\r\n", conv);
    }
  fprintf(stderr, "\r\n");
  
  fscanf(file, " ");//matches any whitespace e.g. newline

  return 1;
}

/*--------------------------------------------------------------------*/
int main(int argc, const char* argv[])
{
  if(argc < 2) 
    {fprintf(stderr, "which file would you like to analyze?\r\n"); exit(-1);}

  while(--argc>0)
    {
      ++argv;
      
      FILE* f = fopen(*argv, "r");
      if(f == NULL)
        {perror(*argv); continue;}
       

      while(read_table(f));
    }
}

