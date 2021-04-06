#ifndef __Params__
#define __Params__ 1

#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

#include "XML.h"

/* all objects (children, attributes, etc..) are considered to be unordered; order will not be preserved */

typedef XML Params;

/*
XML*       xml_new                     (char* top_node_name);
XML*       xml_destroy                 (XML* self);
XML*       xml_open                    (char* path);
BOOL       xml_save                    (XML* self, char* path);
*/

Params* params_new         (char* filename);
char*   params_get_string  (Params* self, char* tag, char*  default_value);
double  params_get_double  (Params* self, char* tag, double deafult_value);
int     params_get_int     (Params* self, char* tag, int    deafult_value);
void    params_set_string  (Params* self, char* tag, char*  value);
void    params_set_double  (Params* self, char* tag, double value);
void    params_set_int     (Params* self, char* tag, int    value);
char*   params_init_string (Params* self, char* tag, char*  default_value);
double  params_init_double (Params* self, char* tag, double default_value);
int     params_init_int    (Params* self, char* tag, int    default_value);
#define params_destroy     xml_destroy

#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif   // __Params__
