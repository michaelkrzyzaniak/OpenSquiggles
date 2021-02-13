#ifndef __XML__
#define __XML__ 1

#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

#include "Hashtable.h"
#include "List.h"

/* all objects (children, attributes, etc..) are considered to be unordered; order will not be preserved */

typedef struct Opaque_XML_Struct       XML;
typedef struct Opaque_XMLNode_Struct   XMLNode;

XML*       xml_new                     (char* top_node_name);
XML*       xml_destroy                 (XML* self);
XML*       xml_open                    (char* path);
BOOL       xml_save                    (XML* self, char* path);

XMLNode*   xml_get_top_node            (XML* self);
List*      xml_get_children            (XMLNode* self); //List* should be treated as read-only and not destroyed by caller
XMLNode*   xml_get_parent              (XMLNode* self);

XMLNode*   xml_new_node                (char* tag, XMLNode* parent);
XMLNode*   xml_destroy_node            (XMLNode* self); //also destroys children; must not destroy top node;
XMLNode*   xml_copy_node               (XMLNode* self, BOOL recursive); //places it in the graph with same parent, unless top node
void       xml_relocate_node           (XMLNode* self, XMLNode* new_parent); //anomalies around top node

List*      xml_get_attributes          (XMLNode* self); //list of hash_entry_t*, can call hash_entry_get_key() and hash_entry_get_value(); List but not entries should be destroyed by caller 
char*      xml_get_attribute           (XMLNode* self, char* key);
BOOL       xml_set_attribute           (XMLNode* self, char* key, char* value); //creates new or overwrites value if key already exists
void       xml_remove_attribute        (XMLNode* self, char* key); 

char*      xml_get_text                (XMLNode* self);
BOOL       xml_set_text                (XMLNode* self, char* text); //whitespace stripped and converted to single spaces
BOOL       xml_append_text             (XMLNode* self, char* text); //single space inserted between old and new text

//these take and return list of XMLNode*. Caller destroys result but not its contents
List*      xml_get_nodes_with_text     (List* nodes, char* text, BOOL recursive);
List*      xml_get_nodes_with_tag      (List* nodes, char* tag , BOOL recursive);
List*      xml_get_nodes_with_attribute(List* nodes, char* key , char* value, BOOL recursive); //value can be NULL

void       xml_sort_nodes_by_text      (List* nodes, BOOL ascending, BOOL recursive);
void       xml_sort_nodes_by_tag       (List* nodes, BOOL ascending, BOOL recursive);
void       xml_sort_nodes_by_attribute (List* nodes, char* key, BOOL ascending, BOOL recursive); //i.e alphabetize by value

#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif   // __XML__
