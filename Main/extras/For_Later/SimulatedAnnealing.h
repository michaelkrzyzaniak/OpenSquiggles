
#ifndef  __MK_ANNEALING__     
#define  __MK_ANNEALING__ 1 

#ifdef __cplusplus            
extern "C" {                
#endif


typedef struct opaque_sa_struct Annealing;

//return distance -- negative indicates undefined cost / unconditional rejection
typedef     double (*sa_cost_funct_t)(void* SELF, double features[]);

//implementations should make some small change to features (already contains current values)
typedef     void   (*sa_make_neighbour_t)(void* SELF, double* features, double temperature);

Annealing*  sa_new                    (int num_features);
Annealing*  sa_destroy                (Annealing* self);

int         sa_get_num_features       (Annealing* self); 
int         sa_init_features          (Annealing* self, double features[], void* callback_self, sa_cost_funct_t cost_funct);
double*     sa_get_features           (Annealing* self);
double*     sa_get_best_features      (Annealing* self);
double      sa_get_best_cost          (Annealing* self);
void        sa_set_alpha              (Annealing* self, double alpha);
double      sa_get_alpha              (Annealing* self);

void        sa_reset_temperature      (Annealing* self);
void        sa_anneal                 (Annealing* self); //multiply temp by alpha;
void        sa_simulate_annealing_once(Annealing*          self,
                                       void*               callback_self, 
                                       sa_cost_funct_t     cost_funct, 
                                       sa_make_neighbour_t neighbour_funct,
                                       int                 successive_rejections_before_reset
                                       ); //does not call sa_anneal    



/* will run at least once and terminate either when max_iterations has been      */
/* reached (ignored if < 1), when min_temp has been reached (ignored if < 0)     */
/* or when the cost is under min_cost (ignored if < 0), or forever.              */
/* returns pointer to the best feature set. Calls sa_anneal() at each generation */
/* will replace the most recently accepted candidate with the best               */
/* candidate once successive_rejections_before_reset number of successive        */
/* rejections occur, unless this value is zero or less                           */

void        sa_simulate_annealing     (Annealing*          self, 
                                       void*               callback_self,
                                       sa_cost_funct_t     cost_funct, 
                                       sa_make_neighbour_t neighbour_funct, 
                                       int                 max_iterations, 
                                       double              min_cost,
                                       double              min_temp,
                                       int                 successive_rejections_before_reset,
                                       double*             result);



#ifdef __cplusplus            
}                             
#endif

#endif//__MK_ANNEALING__