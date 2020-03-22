//gcc quantizer_test.c ../Quantizer.c ../../../Beat*/src/Statistics.c

#include "../Quantizer.h"
#include "../../../Beat-and-Tempo-Tracking/src/Statistics.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define NUM_REALTIME_IOIS  3

void debug_print_iois(Quantizer* quantizer)
{
  unsigned i, n = quantizer_realtime_get_num_IOIs(quantizer);
  double iois[n];
  quantizer_realtime_get_quantized_IOIs(quantizer, iois);
  OnlineAverage* average = online_average_new();
  
  double sum = 0;
  
  fprintf(stderr, "[ ");
  for(i=0; i<n; i++)
    {
      fprintf(stderr, "%.2lf ", iois[i]);
      online_average_update(average, iois[i]);
      sum += iois[i];
    }
  fprintf(stderr, "] (= %02f)\r\n", sum);
  //fprintf(stderr, "mean: %f\tstd_dev: %f\r\n", online_average_mean(average), online_average_std_dev(average));
  average = online_average_destroy(average);
}

int main(void)
{
  Quantizer* quantizer = quantizer_new(NUM_REALTIME_IOIS);

  srandom(time(NULL));


  //  double test_data[NUM_REALTIME_IOIS]  = {-476, 237+476, 115, 135, 174, 135, 155, 240, 254, 118, 112, 118, 138, 476};

  double test_data[NUM_REALTIME_IOIS] = {2.1, -1.1, 2}; //{1, 1, 1}

  int i;
  for(i=0; i<NUM_REALTIME_IOIS; i++)
    {
      //double ioi = statistics_random_normal(100.0, 0.0);
      //quantizer_realtime_push(quantizer, ioi);
      quantizer_realtime_push(quantizer, test_data[i]);
    }
  
  debug_print_iois(quantizer);
  
  double* iois = quantizer_offline_get_quantized_IOIs(quantizer);
  
  for(i=0; i<100; i++)
    {
      quantizer_offline_quantize(quantizer, iois, NUM_REALTIME_IOIS, 1, -1);
      debug_print_iois(quantizer);
    }
  
  quantizer = quantizer_destroy(quantizer);
}

