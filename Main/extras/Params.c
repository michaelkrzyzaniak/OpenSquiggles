#include "Params.h"
#include "List.h"
#include <stdio.h>
#include <string.h>

/*-----------------------------------------------------------------------*/
Params*  params_new(char* settings_xml_path)
{
  Params* self = NULL;
  
  self = xml_open(settings_xml_path);
  if(self != NULL)
    fprintf(stderr, "Loaded xml file: %s\r\n", settings_xml_path);
  else
    self = xml_new("settings");
  
  if(self != NULL)
    {
      params_init_string(self, "xml_save_filename", settings_xml_path);
    }
  return self;
}

/*-----------------------------------------------------------------------*/
char* params_get_string(Params* self, char* tag, char* default_value)
{
  char* result = NULL;
  //XML* xml = self;
  XMLNode* settings_parent =  xml_get_top_node(self);
  List* nodes = xml_get_children(settings_parent);
  List* setting_list = xml_get_nodes_with_tag(nodes, tag, NO);
  XMLNode* setting = NULL;
  if(list_count(setting_list) > 0)
    setting = list_data_at_index(setting_list, 0);
  if(setting != NULL)
    result = xml_get_text(setting);
  list_destroy(setting_list, NO);
  if(result == NULL)
    result = default_value;
  return result;
}

/*-----------------------------------------------------------------------*/
//will return default if tag does not exist
double params_get_double(Params* self, char* tag, double deafult_value)
{
  double result = deafult_value;
  char* str = params_get_string(self, tag, "");
  if(str != NULL)
    if(strlen(str) > 0)
      sscanf(str, "%lf", &result);
  return result;
}

/*-----------------------------------------------------------------------*/
//will return default if tag does not exist
int     params_get_int     (Params* self, char* tag, int    deafult_value)
{
  int result = deafult_value;
  char* str = params_get_string(self, tag, "");
  if(str != NULL)
    if(strlen(str) > 0)
      sscanf(str, "%i", &result);
  return result;
}

/*-----------------------------------------------------------------------*/
void params_set_string(Params* self, char* tag, char* value)
{
  //XML* xml = self->settings;
  XMLNode* settings_parent =  xml_get_top_node(self);
  List* nodes = xml_get_children(settings_parent);
  List* setting_list = xml_get_nodes_with_tag(nodes, tag, NO);
  XMLNode* setting = NULL;
  if(list_count(setting_list) > 0)
    setting = list_data_at_index(setting_list, 0);
  if(setting == NULL)
    setting = xml_new_node (tag, settings_parent);
  if(setting != NULL)
    xml_set_text(setting, value);

  list_destroy(setting_list, NO);
  
  value = params_get_string(self, tag, "");
  fprintf(stderr, "SET -- tag: %s, val: %s\r\n", tag, value);
  
  char* xml_save_filename = params_get_string(self, "xml_save_filename", "");
  if(xml_save_filename != NULL)
    if(strlen(xml_save_filename) > 0)
      xml_save(self, xml_save_filename);
}

/*-----------------------------------------------------------------------*/
void params_set_double(Params* self, char* tag, double value)
{
  char str_value[15];
  snprintf(str_value, 15, "%lf", value);
  params_set_string(self, tag, str_value);
}

/*-----------------------------------------------------------------------*/
void    params_set_int     (Params* self, char* tag, int    value)
{
  char str_value[15];
  snprintf(str_value, 15, "%i", value);
  params_set_string(self, tag, str_value);
}

/*-----------------------------------------------------------------------*/
char*    params_init_string (Params* self, char* tag, char* default_value)
{
  default_value = params_get_string(self, tag, default_value);
  params_set_string(self, tag, default_value);
  return default_value;
}

/*-----------------------------------------------------------------------*/
double    params_init_double  (Params* self, char* tag, double default_value)
{
  default_value = params_get_double(self, tag, default_value);
  params_set_double(self, tag, default_value);
  return default_value;
}

/*-----------------------------------------------------------------------*/
int     params_init_int    (Params* self, char* tag, int    default_value)
{
  default_value = params_get_int(self, tag, default_value);
  params_set_int(self, tag, default_value);
  return default_value;
}
