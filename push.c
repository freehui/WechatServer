#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json.h"
#include "online.h"
#include "client.h"


#define HASH_SIZE (1024)
Client pushclient_hash[HASH_SIZE];

static inline int hash(char *phone){

    unsigned long h=0;
    unsigned long x=0;

    while(*phone)
    {
        h=(h << 4)+(*phone++);
		
        if((x=h & 0xF0000000L) != 0){
			
			h ^= (x>>24);
			h &= ~x;
        }
    }	
    return h % HASH_SIZE;
}

static void add_push_client(Client new,char *phone){
	
	int key = hash(phone);
	 
	Client hea;	
	
	del_client(new->sock);

	if (pushclient_hash[key] == NULL){

		pushclient_hash[key] = new;
		new->next = NULL;
		new->prev = new;
	}else {

		hea = pushclient_hash[key];

		new->prev = hea->prev;
		hea->prev->next = new;
		new->next = NULL;
		hea->prev = new;
	}
}

Client find_push_client(char *phone){
	
	Client c = pushclient_hash[hash(phone)];

	while(c){

		if (strcmp(c->private,phone) == 0)
			break;
		c = c->next;
	}			

	return c;
}

void free_push_client(char *phone){
	
	int key = hash(phone);
	Client c = pushclient_hash[key];
	Client he = c;

	while(c){

		if (strcmp(c->private,phone) == 0){

			c->prev->next = c->next;
			if (c->next){
				c->next->prev = c->prev;
			}else{
				he->prev = c->prev;
				if (pushclient_hash[key] == c){

					pushclient_hash[key] = NULL;
				}
			}
			
			free(c);
			break;
		}
		c = c->next;
	}		
}

int connect_push_server (int client,char *value){
		
	int ret;
	JSON_obj *obj;
	struct online_user *user;
	Client c;
 
	c = find_client(client);
	if (c == NULL)
		return -1;
	//socket 在刚连接时，会创建这个 Client 并插入到一个哈希表内
	//先把他从那个哈希表中删除
	del_client(c->sock);

	obj = parser(value);
	if (obj == NULL)
		return -1;

	user = find_phone(getvalue_bykey(obj,"phone"));	
	if (user == NULL)
		return -1;

	c->private = user->phone;

	add_push_client(c,user->phone);

	free_json(obj);

	return 0;
}

void push_msg(char *phone,char *msg,int len){
	
	Client c = find_push_client(phone);
	if (c == NULL){
		
		return;
	}
	write(c->sock,msg,len);
}

void push_offline_msg(char *phone){
	
	push_msg(phone,"ss",2);
}






