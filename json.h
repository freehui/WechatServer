#ifndef _JSON_H
#define _JSON_H

typedef struct JSON_obj_field {
	
	char *key;
	char *value;
	int v_type;
	struct JSON_obj_field *next;
} JSON_obj_field;

typedef struct JSON_obj{

	struct JSON_obj_field *field_list;
	struct JSON_obj *next;
} JSON_obj;

#define NUMBER (1)
#define STRING (2)

#define is_char(v) (((v > 0x40) && (v < 0x5b)) || ((v > 0x60) && (v < 0x7b)))
#define is_number(v) ((v > 47) && (v < 58))
#define is_white(v) (*v == ' ' || *v == '\t' || *v == '\n')
#define skip_white(v) while(is_white(v))v++

#define JSON_foreach(obj_head,obj_each) \
	for (obj_each = obj_head;obj_each;obj_each = obj_each->next)	
#define JSON_field_foreach(obj,field_each) \
	for (field_each = obj->field_list;field_each;field_each = field_each->next)	

extern JSON_obj *parser(char *text);
extern char *getvalue_bykey(JSON_obj *obj,char *key);
extern void free_json(JSON_obj *obj);

#endif /* _JSON_H */

