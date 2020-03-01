#include "SimulatedAnnealing.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <libc.h>
void sa_copy_features(Annealing* self, double* src, double* dest);

/*-------------------------------------------------------*/
struct opaque_sa_struct
{
  double  T;
  double  alpha;
  double* features;
  double* best_features;
  double  best_cost;
  double  prev_cost;
  int     num_features;
  int     num_successive_rejections;
};

/*-------------------------------------------------------*/
Annealing*  sa_new (int num_features)
{
  Annealing* self = calloc(1, sizeof(*self));
  if(self != NULL)
    {
      self->num_features = num_features;
      self->features = calloc(num_features, sizeof(*(self->features)));
      if(self->features == NULL) return sa_destroy(self);
      self->best_features = calloc(num_features, sizeof(*(self->best_features)));
      if(self->best_features == NULL) return sa_destroy(self);

      self->alpha = 0.9;
      sa_reset_temperature(self);
      srandom(time(NULL));
    }
  return self;
}

/*-------------------------------------------------------*/
Annealing*  sa_destroy                (Annealing* self)
{
  if(self != NULL)
    {
      if(self->features != NULL)
        free(self->features);
      if(self->best_features != NULL)
        free(self->best_features);

      free(self);
    }
  return (Annealing*) NULL;
}

/*-------------------------------------------------------*/
void sa_copy_features(Annealing* self, double* src, double* dest)
{
  int n = self->num_features * sizeof(double);
  memcpy(dest, src, n);
}

/*-------------------------------------------------------*/
int         sa_get_num_features       (Annealing* self)
{
  return self->num_features;
}

/*-------------------------------------------------------*/
void        sa_set_features           (Annealing* self, double features[])
{
  sa_copy_features(self, features, self->features);
}

/*-------------------------------------------------------*/
int sa_init_features          (Annealing* self, double features[], void* callback_self, sa_cost_funct_t cost_funct)
{
  double cost;
  sa_copy_features(self, features, self->features);
  sa_copy_features(self, features, self->best_features);
  cost = cost_funct(callback_self, self->features);
  if(cost < 0) return 0;
  
  self->prev_cost = self->best_cost = cost;
  return 1;
}

/*-------------------------------------------------------*/
double*     sa_get_features           (Annealing* self)
{
  return self->features;
}

/*-------------------------------------------------------*/
double*     sa_get_best_features      (Annealing* self)
{
  return self->best_features;
}

/*-------------------------------------------------------*/
double sa_get_best_cost               (Annealing* self)
{
  return self->best_cost;
}

/*-------------------------------------------------------*/
void        sa_set_alpha              (Annealing* self, double alpha)
{
  self->alpha = alpha;
}

/*-------------------------------------------------------*/
double      sa_get_alpha              (Annealing* self)
{
  return self->alpha;
}

/*-------------------------------------------------------*/
void        sa_reset_temperature      (Annealing* self)
{
  self->T = 1;
}

/*-------------------------------------------------------*/
void        sa_anneal                 (Annealing* self)
{
  self->T *= self->alpha;
}

/*-------------------------------------------------------*/
void        sa_simulate_annealing_once(Annealing*          self,
                                       void*               callback_self, 
                                       sa_cost_funct_t     cost_funct, 
                                       sa_make_neighbour_t neighbour_funct, 
                                       int                 successive_rejections_before_reset)   
{
  double candidate[self->num_features];
  double cost;
  int was_accepted = 0;
  int was_best     = 0;
  
  if(successive_rejections_before_reset > 0)
    if(self->num_successive_rejections >= successive_rejections_before_reset)
      {
        sa_copy_features(self, self->best_features, self->features);
        self->num_successive_rejections = 0;
        self->prev_cost = self->best_cost;
        fprintf(stderr, "resetting to best features after too many rejections\r\n");
      }

  sa_copy_features(self, self->features, candidate);
  neighbour_funct(callback_self, candidate, self->T);
  cost = cost_funct(callback_self, candidate);

  if(cost >= 0)
    {
      //what if T is < 0
      double p;
      if(self->T > 0)
        p = exp((self->prev_cost - cost) / self->T);
      else
        p = ((self->prev_cost - cost) > 0) ? 2 : 0; //2 or anything > 1
      double r = random() / (long double)RAND_MAX;
      
      //we are trying to minimize the cost..
      if(p > r)
        {
          self->prev_cost = cost;
          sa_copy_features(self, candidate, self->features);
          was_accepted = 1;
          if(cost < self->best_cost)
            {
              self->best_cost = cost;
              sa_copy_features(self, candidate, self->best_features);
              was_best = 1;
            }
          
        }
    }
    
  if(was_best)
    self->num_successive_rejections = 0;
  else 
    self->num_successive_rejections++;
    
  if(was_accepted)
    fprintf(stderr, "accept!\r\n");
  else 
    fprintf(stderr, "reject!\r\n");
}


/* will run at least once and terminate either when max_iterations has been     */
/* reached (ignored if < 1), when min_temp has been reached (ignored if < 0)    */
/* or when the change in cost is under min_change (ignored if < 0), or forever. */
/* returns pointer to the best feature set.                                     */
/* will replace the most recently accepted candidate with the best              */
/* candidate once successive_rejections_before_reset number of successive       */
/* rejections occur, unless this value is zero or less                          */
/*-------------------------------------------------------*/
void        sa_simulate_annealing     (Annealing*          self, 
                                       void*               callback_self,
                                       sa_cost_funct_t     cost_funct, 
                                       sa_make_neighbour_t neighbour_funct, 
                                       int                 max_iterations, 
                                       double              min_cost,
                                       double              min_temp,
                                       int                 successive_rejections_before_reset,
                                       double*             result)
{
  int i=0;
  double deriv;
  self->num_successive_rejections = 0;
  for(;;)
    {
      sa_simulate_annealing_once(self, callback_self, cost_funct, neighbour_funct, successive_rejections_before_reset);

      if(min_cost >= 0)
        if(self->best_cost < min_cost)
          break;

      if(min_temp >= 0)
        if(self->T < min_temp)
          break;

      ++i;
      if(max_iterations > 0)
        if(i >= max_iterations)
          break;

      sa_anneal(self);
    }
  if(result != NULL)
    sa_copy_features(self, sa_get_best_features(self), result);
}
