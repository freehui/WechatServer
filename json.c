#include <stdio.h>
#include <malloc.h>
#include "json.h"



/*
 * 解析 JSON 数组。返回一个存放解析结果的 JSON_obj 链表。
 * 
 * text: 不能是常量字符串，必须可写
 * 
 * 支持如下格式的 JSON 字符串
 * 1. [{"a":123}]
 * 2. [{"a":"key"}]
 * 3. [{"a1":"key1","a2":"key2","a3":456}]
 * 4. [{"a1":123,"a2":"key"},{"b1":"dfds"},{"fgh":123,"ert":"fdg"}]
 * 
*/
JSON_obj *parser(char *text){
	
	char *json_text = text;
	JSON_obj *head;
	JSON_obj *prev;
	JSON_obj *current;
	JSON_obj_field *field;	
	int dian;

	head = malloc(sizeof(JSON_obj));
	prev = current = head;
	head->field_list = NULL;

	skip_white(json_text);
	if (*json_text++ != '['){
		free_json(head);	
		return NULL;
	}

	for (;;){
		/* 解析 JSON 数组内每个 JSON 对象 */
		
		skip_white(json_text);
		if (*json_text++ != '{' && *json_text == ':'){
			free_json(head);
			return NULL;		
		}

		for (;;){ 
			/* 解析 JSON 对象内每个 JSON 成员 */
			
			field = malloc(sizeof(JSON_obj_field));
			field->next = current->field_list;
			current->field_list = field;

			skip_white(json_text);
			if (*json_text++ != '\"'){
				free_json(head);
				return NULL;
			}
		
			field->key = json_text;/* key */  
			while(*json_text != '\"'){
			
				if (is_white(json_text)){
					free_json(head);					
					return NULL;
				}
				json_text++;	
			}

			*json_text = '\0';
			json_text++; /* key */
		
			skip_white(json_text);

			if (*json_text++ != ':'){
				free_json(head);			
				return NULL;
			}

			skip_white(json_text);

			if (*json_text == '\"'){
				/* "xxx" : "yyy"  */

				field->value = json_text = json_text + 1;	

				while(*json_text != '\"')/* skip */
					if (*json_text != '\0')					
						json_text++;	
					else {	// [{"xxx" : "yyy }]
						free_json(head);
						return NULL;
					}
	
				*json_text++ = '\0';
				skip_white(json_text); 

				if (*json_text == '}') /* fields end */
					break;

		        if (*json_text != ',') {
					free_json(head);				
					return NULL;
				}
		
				field->v_type = STRING;

				/* 现在,*json_text 必然 == ',' */
			}else {
				/* "xxx" : 123 */
			
				field->value = json_text;	

				if (!is_number(*json_text)) /* 数字必须以数字开头 */{
					free_json(head);				
					return NULL;
				}
				
				dian = 0;
				/* 必须是数字或小数点,且小数点只能有 1 个 */
				while( (is_number(*json_text) || ((*json_text == '.') && (dian == 0) && (dian = 1))) && 
				       !is_white(json_text) &&  
				       *json_text != '}' &&
				       *json_text != ','){

					if (is_char(*json_text)){
						free_json(head);					
						return NULL;
					}
					json_text++;		
				}
				
				if (is_white(json_text)){
					*json_text++ = '\0';
					skip_white(json_text);
				}

				if (*json_text == '}'){
					*json_text = '\0';
					break;
				}

				if (*json_text != ','){
					free_json(head);				
					return NULL;
				}
				
				*json_text = '\0';
				field->v_type = NUMBER;

				/* 现在,*json_text 必然 == ',' */
			}	
					
			/* json_text 必须是 ','. 现在跳过 ',' */
			json_text++;
		}

		/* 现在必须是 '}'，跳过他 */
		json_text++; 
		skip_white(json_text);

		if (*json_text == ','){
			/* [...{...},{...}...] */
			
			json_text++;

			skip_white(json_text);
			if (*json_text != '{'){
				free_json(head);
				return NULL; 
			}
		}else if (*json_text == ']'){
			/* [...{...}] */
			json_text++;
			while(*json_text != '\0')
				if (*json_text++ != ' '){
					free_json(head);
					return NULL;
				}
			break;
		}else {
			free_json(head);
			return NULL;
		}
	
		current = malloc(sizeof(struct JSON_obj));
		current->next = NULL;
		current->field_list = NULL;
		prev->next = current;
		prev = current;
	}	

	current->next = NULL;
	return head;
}

void free_json(JSON_obj *head){

	JSON_obj *each;	
	JSON_obj_field *field;
		
	JSON_foreach(head,each){

		JSON_field_foreach(each,field){
			free(field);
		}
		free(each);
	}	
}

/*
 * 传入一个 JSON_obj 和要取的 key，返回 value 值。
*/
char *getvalue_bykey(JSON_obj *obj,char *key){
	
	JSON_obj_field *field = obj->field_list;
		
	while(field){
		
		if (strcmp(field->key,key) == 0)
			return field->value;
		
		field = field->next;
	}

	return NULL;
}


/*
 * example
*/
#if 0
int main(void){
	
	JSON_obj *head;
	JSON_obj *each;
	JSON_obj_field *field;
	char str[] = "[{\"ssss\":\"jjjj}]";
	head = parser(str);
	if (head == NULL){
			
		
		printf("error:it is not json format!\n");
		return 0;	
	}
	/* 遍历数组内所有对象并打印出他们的 key 和 value. */
	JSON_foreach(head,each){
		
		JSON_field_foreach(each,field){
			
			printf("key = %s,value = %s\n",field->key,field->value);
		}
		printf(".................\n");
	}

	printf("~~ %s ~~\n",getvalue_bykey(head,"abc")); 

	return 0;
}
#endif
