#include <string.h>
#include <malloc.h>
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include "online.h"
#include "client.h"

#define HASH_SIZE (1024)
struct online_user *online_user_hash[HASH_SIZE];
struct online_phone *online_phone_hash[HASH_SIZE];

static inline int hash(int sock){

	return sock % HASH_SIZE;
}

inline int elfhash(char *phone){

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


struct online_user * find_user(int sock){

	struct online_user *ou = online_user_hash[hash(sock)];
	while(ou){

		if (ou->sock == sock)
			break;
		ou = ou->next;
	}	

	return ou;
}

struct online_user * find_phone(char *phone){

	struct online_phone *ou = online_phone_hash[elfhash(phone)];

	while(ou){

		if (strcmp(ou->user->phone,phone) == 0)
			break;
		ou = ou->next;
	}	

	return ou ? ou->user : NULL;
}


/*
 * 插入一个在线用户
 */
//struct online_user *online_user(int sock,char *phonenumber,char *name){
struct online_user *online_user(struct online_user *new){
	
	Client client = get_client(new);
	int key = hash(client->sock);
	int pkey = elfhash(client->user.phone);
	struct online_phone *pnew = malloc(sizeof(struct online_phone));
	struct online_user *hea;	
	struct online_phone *phea;	
	struct sockaddr_in sa;
	int len = sizeof(sa);
	pnew->user = new;	
	new->handler = NULL;
	new->private = NULL;	
	new->pptr = pnew;
	if(!getpeername(client->sock, (struct sockaddr *)&sa, &len)){
		strcpy(new->ip,inet_ntoa(sa.sin_addr));
	}		
	
	if (online_user_hash[key] == NULL){

		online_user_hash[key] = new;
		online_phone_hash[pkey] = pnew;
		new->next = NULL;
		new->prev = new;
		pnew->next = NULL;
		pnew->prev = pnew;		
	}else {

		hea = online_user_hash[key];
		phea = online_phone_hash[key];
		
		new->prev = hea->prev;
		hea->prev->next = new;
		new->next = NULL;
		hea->prev = new;

		pnew->prev = phea;
		phea->prev->next = pnew;
		pnew->next = NULL;
		phea->prev = pnew;
	}

	return new;
}


/*
 * 下线一个用户
 */
void offline_user(int sock){

	int key = hash(sock);
	struct online_user *ou = online_user_hash[key];
	if (ou == NULL)
		return;
	
	struct online_user *he = ou;
	struct online_phone *pou = ou->pptr;
	struct online_phone *phe = pou;
	
	while(ou){

		if (ou->sock == sock){

			ou->prev->next = ou->next;
			pou->prev->next = pou->next;
			
			if (ou->next){
				ou->next->prev = ou->prev;
				pou->next->prev = pou->prev;
			}else{
				he->prev = ou->prev;
				phe->prev = pou->prev;
				if (online_user_hash[key] == ou){

					online_user_hash[key] = NULL;
					online_phone_hash[elfhash(ou->phone)] = NULL;
				}
			}
			
			free(pou);
			return;			
		}else{

			ou = ou->next;
			pou = pou->next;
		}
	}
}

/*
 * 下线心跳包超时的所有用户
 */
void offline_outtime_users(void){

	struct online_user * ou;
	struct online_user * temp;
	int i;

	for (i = 0;i < HASH_SIZE;i++){

		ou = online_user_hash[i];
		if (ou == NULL)
			continue;
		
		while(ou){

			temp = ou->next;
			offline_user(ou->sock);
			ou = temp;
		}
			;
	}
}


