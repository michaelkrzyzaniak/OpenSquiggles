#include "XML.h"
#include <stdio.h>
#include <string.h>

XMLNode*   _xml_destroy_node        (XMLNode* self);

/*-------------------------------------------------------------------------------------*/
struct Opaque_XMLNode_Struct
{
  char*      tag;
  char*      text;
  XMLNode*   parent;
  List*      children;
  Hashtable* attributes;
};

/*-------------------------------------------------------------------------------------*/
struct Opaque_XML_Struct
{
  XMLNode* top_node;
};

/*-------------------------------------------------------------------------------------*/
char* xml_copy_string(char* str)
{
 size_t len = strlen(str) + 1;
 char*  copy  = malloc(len);
 if(copy != NULL)
   strcpy(copy, str);
  return copy;
}

/*-------------------------------------------------------------------------------------*/
void xml_trim_string(char* str)
{
  size_t len = strlen(str);
  int i;
  char* c = str;
  
  for(i=0; i<len; i++)
    if(*c > 0x20)
      break;
    else
      ++c;
      
  i = c-str;
  len -= i;
  if(i != 0)
    memmove(str, c, len + 1);
  
  c = str + len - 1;
  for(i=0; i<len; i++)
    if(*c > 0x20)
      break;
    else
      --c;
  c[1] = '\0';
}

/*-------------------------------------------------------------------------------------*/
void  xml_whitespace_to_single_space(char* str)
{
  while(*str != '\0')
    {
      if(*str <= 0x20)
        {
          *str = ' ';
          xml_trim_string(str+1);
        }
      ++str;
    }
}

/*-------------------------------------------------------------------------------------*/
void  xml_replace_char(char* str, char old, char new)
{
  while(*str != '\0')
    {
      if(*str == old)
        *str = new;
      ++str;
    }
}

/*-------------------------------------------------------------------------------------*/
void  xml_clean_text(char* str)
{
  xml_trim_string(str);
  xml_whitespace_to_single_space(str);
}

/*-------------------------------------------------------------------------------------*/
/*
void  xml_clean_attribute_key(char* str)
{
  xml_trim_string(str);
  xml_whitespace_to_single_space(str);
  xml_replace_char(str, ' ', '_');
}
*/

/*-------------------------------------------------------------------------------------*/
void  xml_clean_attribute_value(char* str)
{
  xml_trim_string(str);
  xml_whitespace_to_single_space(str);
  xml_replace_char(str, 0x22, 0x27);
}

/*-------------------------------------------------------------------------------------*/
XML*       xml_new                 (char* top_node_tag)
{
  XML* self = calloc(1, sizeof(*self));
  
  if(self != NULL)
    {
      if(top_node_tag != NULL)
        {
          self->top_node = xml_new_node(top_node_tag, NULL);
          if(self->top_node == NULL)
            return xml_destroy(self);
        }
    }
  
  return self;
}

/*-------------------------------------------------------------------------------------*/
XML*       xml_destroy             (XML* self)
{
  if(self != NULL)
    {
      _xml_destroy_node(self->top_node);
      free(self);
    }
  return (XML*) NULL;
}

/*-------------------------------------------------------------------------------------*/
XMLNode*   xml_new_node            (char* tag, XMLNode* parent /* can be NULL, (i.e. for top level tag) */)
{
  XMLNode* self = calloc(1, sizeof(*self));
  //fprintf(stderr, "new node: %s\r\n", tag);
  if(self != NULL)
    {
      self->tag = xml_copy_string(tag);
      if(self->tag == NULL)
        return xml_destroy_node(self);
      self->parent = parent;
      self->children = list_new();
      if(self->children == NULL)
        return xml_destroy_node(self);
      self->attributes = hash_new(10);
      if(self->attributes == NULL)
        return xml_destroy_node(self);
      self->text = calloc(1, 1);
      if(self->text == NULL)
        return xml_destroy_node(self);
      
      if(self->parent != NULL)
        list_append_data(self->parent->children, self, (list_data_deallocator_t)_xml_destroy_node);
    }
  
  return self;
}

/*-------------------------------------------------------------------------------------*/
XMLNode*   xml_copy_node           (XMLNode* self, BOOL recursive)
{
  //what if self is top node?
  XMLNode* copy = xml_new_node(self->tag, self->parent);
  if(copy != NULL)
    {
      list_iterator_t i;
      List* attributes = xml_get_attributes(self);
      
      if(attributes != NULL)
        {
          i = list_reset_iterator(attributes);
          hash_entry_t* attribute;
          while((attribute = list_advance_iterator(attributes, &i)) != NULL)
            xml_set_attribute(copy, hash_entry_get_key(attribute), hash_entry_get_value(attribute));
          attributes = list_destroy(attributes, NO);
        }
      xml_set_text(copy, xml_get_text(self));
      
      if(recursive)
        {
          i = list_reset_iterator(self->children);
          XMLNode* node;
          while((node = list_advance_iterator(self->children, &i)) != NULL)       
            xml_copy_node(node, recursive);
        }
    }
  return copy;
}

/*-------------------------------------------------------------------------------------*/
void       xml_relocate_node       (XMLNode* self, XMLNode* new_parent)
{
  if(self->parent != NULL)
    list_remove_data(self->parent->children, self, NO);
  list_append_data(new_parent->children, self, (list_data_deallocator_t)_xml_destroy_node);
  self->parent = new_parent;
}

/*-------------------------------------------------------------------------------------*/
XMLNode*   _xml_destroy_node        (XMLNode* self)
{
  if(self != NULL)
    {
      self->children = list_destroy(self->children, YES);
      if(self->tag != NULL)
        free(self->tag);
      hash_destroy(self->attributes, YES);
      if(self->text != NULL)
        free(self->text);
      free(self);
    }
  return (XMLNode*) NULL;
}

/*-------------------------------------------------------------------------------------*/
XMLNode*   xml_destroy_node        (XMLNode* self)
{
  if(self != NULL)
    {
    
      _xml_destroy_node(self);
      if(self->parent != NULL)
        list_remove_data(self->parent->children, self, NO);
    }
  return (XMLNode*) NULL;
}

/*-------------------------------------------------------------------------------------*/
char* xml_file_to_string(FILE* f, size_t len /* not inculding '\0' */)
{
  char* str = malloc(len+1);
  if(str != NULL)
    {
      fseek(f, -(len+1), SEEK_CUR);
      //int n = (int)fread(str, 1, len, f);
      fread(str, 1, len, f);
      fseek(f, 1, SEEK_CUR);
      str[len] = '\0';
      
    }
  return str;
}

/*-------------------------------------------------------------------------------------*/
int xml_scan_whitespace(FILE* f, BOOL should_rewind_last_char)
{
  int c;
  while((c = fgetc(f)) != EOF)
    {
      if(c > 0x20)
        break;
    }
  if(should_rewind_last_char)
    fseek(f, -1, SEEK_CUR);
    
  return c;
}

/*-------------------------------------------------------------------------------------*/
//return 0 if close tag
//tag == NULL if error
int xml_scan_tagname(FILE* f, char** tag)
{
  int is_open = 1;
  int c, len=0;
  *tag = NULL;
  
  fgetc(f); //guaranteed to be '<'
  
  c = xml_scan_whitespace(f, NO);
  if(c == '/')
    is_open = 0;
  else
    fseek(f, -1, SEEK_CUR);
  
  xml_scan_whitespace(f, YES);
  
  for(;;)
    {
      c = fgetc(f);
      if((c == '<') || (c == EOF))
        break;
      else if((c <= 0x20) || (c == '>')) //this must come after EOF b/c <= 0x20
        { 
          *tag = xml_file_to_string(f, len);
          if(c == '>') 
            fseek(f, -1, SEEK_CUR);
          break;
        }
      else
        ++len;
    }
    
  return is_open;
}

/*-------------------------------------------------------------------------------------*/
void xml_scan_attribute(FILE* f, char** key, char** value)
{
  int c, len=0;
  *key = *value = NULL;
  char quote;
  
  //guaranteed no whitespace at begining  
  for(;;)
    {
      c=fgetc(f);
      if((c == '<') || (c == EOF) || (c == '>'))
        goto out;
      else if((c <= 0x20) || (c == '=')) //this must come after EOF b/c <= 0x20
        {
          *key = xml_file_to_string(f, len);
          break;
        }
      else
        ++len;
    }
  
  if(c != '=')
    c = xml_scan_whitespace(f, NO);
  if(c != '=')
    goto out;
  xml_scan_whitespace(f, YES );
  quote = fgetc(f);
  if((quote != 0x22) && (quote != 0x27))
    goto out;

  len = 0;
  for(;;)
    {
      c=fgetc(f);
      if(c == EOF)
        goto out;
      else if(c == quote)
        {
          *value = xml_file_to_string(f, len);
          break;
        }
      else
        ++len;
    }
    
 out:
  if(*value == NULL)
    if(*key != NULL)
      {free(*key); *key = NULL;}
}

/*-------------------------------------------------------------------------------------*/
char* xml_scan_text(FILE* f)
{
  int c, len=0;
  
  for(;;)
    {
      c=fgetc(f);
      if(c == EOF)
        return NULL;
      else if(c == '<')
        {
          char* result = xml_file_to_string(f, len);
          fseek(f, -1, SEEK_CUR);
          return result;
        }
      else
        ++len;
    }
}

/*-------------------------------------------------------------------------------------*/
XML*       xml_open                (char* path)
{
  XML* self = xml_new(NULL);
  int c;
  char *str, *key, *value;
  
  if(self != NULL)
    {
      XMLNode* parent = NULL;
      FILE* f = fopen(path, "r");
      if(f != NULL)
        {
          c = xml_scan_whitespace(f, YES);
          if(c != '<')
            {fclose(f); return xml_destroy(self);}
          for(;;)
            {
              c = xml_scan_whitespace(f, YES);
              if(c == '<')
                {
                  BOOL is_open_tag = xml_scan_tagname(f, &str);
                  if(str == NULL)
                    {fclose(f); return xml_destroy(self);}
                  if(is_open_tag)
                    {
                      XMLNode* node = xml_new_node(str, parent);
                      if(node == NULL)
                        {fclose(f); fprintf(stderr, "stack overflow while reading %s\r\n", path); return xml_destroy(self);}
                      if(parent == NULL)
                        self->top_node = node;
                      parent = node;
                      
                      for(;;)
                        {
                          c = xml_scan_whitespace(f, YES);
                          if(c == '>')
                            {
                              c = fgetc(f);
                              break;
                            }
                          else
                            {
                              xml_scan_attribute(f, &key, &value);
                              if(key == NULL)
                                {fclose(f); fprintf(stderr, "unable to read attribute for tag %s in file %s\r\n", parent->tag, path); return xml_destroy(self);}
                              xml_set_attribute(parent, key, value);
                              free(key); free(value);
                            }
                        }
                    }
                  else //(is_close_tag)
                    {
                      c = xml_scan_whitespace(f, NO);
                      if(c != '>')
                        {fclose(f); fprintf(stderr, "improperly terminated close tag for %s in %s\r\n", parent->tag, path); return xml_destroy(self);}
                      if(strcmp(str, parent->tag) != 0)
                        {fclose(f); fprintf(stderr, "improperly nested tags in %s\r\n", path); return xml_destroy(self);}
                      if(parent == self->top_node) 
                        {break; fprintf(stderr, "finished scanning file: %s\r\n", path);}
                      else parent = parent->parent;
                    }
                  if(str != NULL)
                    free(str);
                }
              else if (c == EOF)
                {
                  if(parent != self->top_node)
                    {fclose(f); fprintf(stderr, "EOF reached without closing top-level tag in %s\r\n", path); return xml_destroy(self);}
                  else break;
                }
              else
                {
                  str = xml_scan_text(f);
                  if(str == NULL)
                    {fclose(f); fprintf(stderr, "error while reading text in node %s in file %s \r\n", parent->tag, path); return xml_destroy(self);}
                  xml_append_text(parent, str);
                  free(str);
                }
            }
          
          fclose(f);
        }
      else
        {
          perror("unable to open XML file");
          return xml_destroy(self);
        }
    }
  return self;
}

/*-------------------------------------------------------------------------------------*/
void       xml_save_subgraph       (XMLNode* self, int depth, FILE* f)
{
  int i;
  //char* str;
  List* l = self->children;
  list_iterator_t iterator;
  list_data_t child, attribute;


  for(i=0; i<depth*2; i++)
    fputc(' ', f);
  fprintf(f, "<%s", self->tag);
  
  l = hash_get_all_entries(self->attributes);
  if(l != NULL)
    {
      iterator = list_reset_iterator(l);
      while((attribute = list_advance_iterator(l, &iterator)) != NULL)
        {
          char* key   = hash_entry_get_key  (attribute);
          char* value = hash_entry_get_value(attribute);
          fprintf(f, " %s=\"%s\"", key, value);
        }
      list_destroy(l, NO);
    }
    
  fprintf(f, ">\r\n");
  
  if(self->text[0] != '\0')
    {
      for(i=0; i<(depth+1)*2; i++)
        fputc(' ', f);  
      fprintf(f, "%s\r\n", self->text);
    }
      
  l = self->children;
  iterator = list_reset_iterator(l);
  while((child = list_advance_iterator(l, &iterator)) != NULL)
    xml_save_subgraph (child, depth+1, f);
  
  for(i=0; i<depth*2; i++)
    fputc(' ', f);
  fprintf(f, "</%s>\r\n", self->tag);  
  
}

/*-------------------------------------------------------------------------------------*/
BOOL       xml_save                (XML* self, char* path)
{
  FILE* f = fopen(path, "wb+");
  BOOL success = NO;
  
  if(f != NULL)
    {
      xml_save_subgraph(self->top_node, 0, f);
      fclose(f);
      success = YES;
    }
  return success;
}

/*-------------------------------------------------------------------------------------*/
XMLNode*   xml_get_top_node        (XML* self)
{
  return self->top_node;
}

/*-------------------------------------------------------------------------------------*/
List*      xml_get_children        (XMLNode* self)
{
  return self->children;
}

/*-------------------------------------------------------------------------------------*/
XMLNode*   xml_get_parent          (XMLNode* self)
{
  return self->parent;
}

/*-------------------------------------------------------------------------------------*/
void     _xml_get_nodes_with_text    (List* nodes, char* text, BOOL recursive, List* result)
{
  list_iterator_t i;
  XMLNode* node;
  i = list_reset_iterator(nodes);
  while((node = list_advance_iterator(nodes, &i)) != NULL)
    {
      if(strcmp(text, node->text) == 0)
        list_append_data(result, node, NULL);
      if(recursive)
        _xml_get_nodes_with_text(node->children, text, recursive, result);
    }
}

/*-------------------------------------------------------------------------------------*/
List*      xml_get_nodes_with_text    (List* nodes, char* text, BOOL recursive)
{
  List* result = list_new();
  if(result != NULL)
    _xml_get_nodes_with_text(nodes, text, recursive, result);
  return result;
}

/*-------------------------------------------------------------------------------------*/
void      _xml_get_nodes_with_tag     (List* nodes, char* tag, BOOL recursive, List* result)
{
  list_iterator_t i;
  XMLNode* node;
  i = list_reset_iterator(nodes);
  while((node = list_advance_iterator(nodes, &i)) != NULL)
    {
      if(strcmp(tag, node->tag) == 0)
        list_append_data(result, node, NULL);
      if(recursive)
        _xml_get_nodes_with_tag(node->children, tag, recursive, result);
    }
}

/*-------------------------------------------------------------------------------------*/
List*    xml_get_nodes_with_tag     (List* nodes, char* tag, BOOL recursive)
{
  List* result = list_new();
  if(result != NULL)
    _xml_get_nodes_with_tag(nodes, tag, recursive, result);
  return result;
}

/*-------------------------------------------------------------------------------------*/
void      _xml_get_nodes_with_attribute(List* nodes, char* key, char* value, BOOL recursive, List* result)
{
  list_iterator_t i;
  XMLNode* node;
  char* v;
  
  i = list_reset_iterator(nodes);
  while((node = list_advance_iterator(nodes, &i)) != NULL)
    {
      v = xml_get_attribute(node, key);
      if(v != NULL)
        {
          if(value == NULL)
            list_append_data(result, node, NULL);
          else
            if(strcmp(v, value) == 0)
              list_append_data(result, node, NULL);
        }
      if(recursive)
        _xml_get_nodes_with_attribute(node->children, key, value, recursive, result);
    }
}

/*-------------------------------------------------------------------------------------*/
List*       xml_get_nodes_with_attribute(List* nodes, char* key, char* value, BOOL recursive)
{
  List* result = list_new();
  if(result != NULL)
    _xml_get_nodes_with_attribute(nodes, key, value, recursive, result);
  return result;
}

/*-------------------------------------------------------------------------------------*/
List* xml_get_attributes      (XMLNode* self)
{
  return hash_get_all_entries(self->attributes);
}

/*-------------------------------------------------------------------------------------*/
char*      xml_get_attribute        (XMLNode* self, char* key)
{
  return hash_get_value_for_key(self->attributes, key);
}

/*-------------------------------------------------------------------------------------*/
void       xml_remove_attribute     (XMLNode* self, char* key)
{
  hash_remove_value_for_key(self->attributes, key, YES);
}

/*-------------------------------------------------------------------------------------*/
void* xml_value_deallocator(void* self)
{
  if(self != NULL)
    free(self);
  return NULL;
}
/*-------------------------------------------------------------------------------------*/
BOOL       xml_set_attribute        (XMLNode* self, char* key, char* value)
{
  char* val = xml_copy_string(value);
  xml_clean_attribute_value(val);
  //xml_clean_attribute_key(???); //todo: key could be const, so should clean copied version.
  if(val != NULL)
    {
      xml_remove_attribute     (self, key);
      hash_store_value_for_key(self->attributes, key, val, xml_value_deallocator);
    }
  return YES;
}

/*-------------------------------------------------------------------------------------*/
char*      xml_get_text            (XMLNode* self)
{
  return self->text;
}

/*-------------------------------------------------------------------------------------*/
BOOL       xml_set_text            (XMLNode* self, char* text)
{
  BOOL success = NO;
  char* t = xml_copy_string(text);
  if(t != NULL)
    {
      xml_clean_text(t);
      char* old = self->text;
      self->text = t;
      free(old);
      success = YES;
    }
  return success;
}

/*-------------------------------------------------------------------------------------*/
BOOL       xml_append_text         (XMLNode* self, char* text)
{
  size_t len1 = strlen(self->text);
  size_t len2 = strlen(text);
  char* new_self_text = realloc(self->text, len1 + len2 + 2);
  if(new_self_text != NULL)
    {
      new_self_text[len1] = ' ';
      memcpy(new_self_text + len1 + 1, text, len2+1);
      if(len1 > 0)
        xml_clean_text(new_self_text + (len1-1));
      else
        xml_clean_text(new_self_text);
      self->text = new_self_text;
    }
    
  return YES;
}

/*-------------------------------------------------------------------------------------*/
BOOL       xml_sort_compare_strings (char* str1, char* str2, BOOL ascending)
{
  BOOL result;
  
  if(str1 == NULL)
    result = NO;
  else if (str2 == NULL)
    result = YES;
  else
    {
      int n = strcmp(str1, str2);
      result = (n > 0);
    }
    
  if(!ascending) result = !result;
  return result;
}

/*-------------------------------------------------------------------------------------*/
BOOL      xml_sort_by_text_callback (XMLNode* node_a, XMLNode* node_b, BOOL ascending, void* ignored)
{
  return xml_sort_compare_strings (node_a->text, node_b->text, ascending);
}

/*-------------------------------------------------------------------------------------*/
void       xml_sort_nodes_by_text     (List* nodes, BOOL ascending, BOOL recursive)
{
  list_sort(nodes, (list_sort_callback_t)xml_sort_by_text_callback, ascending, NULL);

  if(recursive)
    {
      list_iterator_t i = list_reset_iterator(nodes);
      XMLNode* node;
      while((node = list_advance_iterator(nodes, &i)) != NULL)
        xml_sort_nodes_by_text (xml_get_children(node), ascending, recursive);
    }
}

/*-------------------------------------------------------------------------------------*/
BOOL      xml_sort_by_tag_callback (XMLNode* node_a, XMLNode* node_b, BOOL ascending, void* ignored)
{
  return xml_sort_compare_strings (node_a->tag, node_b->tag, ascending);
}

/*-------------------------------------------------------------------------------------*/
void       xml_sort_nodes_by_tag      (List* nodes, BOOL ascending, BOOL recursive)
{
  list_sort(nodes, (list_sort_callback_t)xml_sort_by_tag_callback, ascending, NULL);

  if(recursive)
    {
      list_iterator_t i = list_reset_iterator(nodes);
      XMLNode* node;
      while((node = list_advance_iterator(nodes, &i)) != NULL)
        xml_sort_nodes_by_tag (xml_get_children(node), ascending, recursive);
    }
}

/*-------------------------------------------------------------------------------------*/
BOOL      xml_sort_by_attribute_callback (XMLNode* node_a, XMLNode* node_b, BOOL ascending, char* key)
{
  char* a = hash_get_value_for_key(node_a->attributes, key);
  char* b = hash_get_value_for_key(node_b->attributes, key);
  return xml_sort_compare_strings (a, b, ascending);
}

/*-------------------------------------------------------------------------------------*/
void       xml_sort_nodes_by_attribute (List* nodes, char* key, BOOL ascending, BOOL recursive)
{
  list_sort(nodes, (list_sort_callback_t)xml_sort_by_attribute_callback, ascending, key);
  
  if(recursive)
    {
      list_iterator_t i = list_reset_iterator(nodes);
      XMLNode* node;
      while((node = list_advance_iterator(nodes, &i)) != NULL)
        xml_sort_nodes_by_attribute (xml_get_children(node), key, ascending, recursive);
    }
}






















