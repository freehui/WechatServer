#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/epoll.h>
#include "client.h"


#define HASH_SIZE (1024)
Client clients_hash[HASH_SIZE];


static inline int hash(int sock){

	return sock % HASH_SIZE;
}

Client alloc_client(int sock){
	
	Client new = malloc(sizeof(*new));
	memset(new,0,sizeof(*new));
	new->sock = sock;
	return new;
}

void add_client(Client new){
	
	int key = hash(new->sock);
	 
	Client hea;	
	if (clients_hash[key] == NULL){

		clients_hash[key] = new;
		new->next = NULL;
		new->prev = new;
	}else {

		hea = clients_hash[key];

		new->prev = hea->prev;
		hea->prev->next = new;
		new->next = NULL;
		hea->prev = new;
	}
}

Client find_client(int sock){
	
	Client ou = clients_hash[hash(sock)];
	while(ou){
		if (ou->sock == sock)
			break;
		ou = ou->next;
	}	

	return ou;
}


/* 删掉一个 Client，并返回删掉的 Client */
Client del_client(int sock){
	
	int key = hash(sock); 
	Client ou = clients_hash[key];
	if (ou == NULL)
		return NULL;
	
	Client he = ou;

	while(ou){

		if (ou->sock == sock){

			ou->prev->next = ou->next;
			
			if (ou->next){
				ou->next->prev = ou->prev;
			}else{
				he->prev = ou->prev;
				if (clients_hash[key] == ou){

					clients_hash[key] = NULL;
				}
			}
			
			break;			
		}else{

			ou = ou->next;
		}
	}

	return ou;
}


/*
 * 下线一个用户
 */
void free_client(int sock){
	
	offline_user(sock);
	free(del_client(sock));
}


void close_all_connect(void){
	
	int i;
	Client c;
	Client n;
	for (i = 0;i < HASH_SIZE;i++){
	
		c = clients_hash[i];
		if (c == NULL)
			continue;

		do {
		
			n = c->next;
			close_connect(c->sock);
		} while(n);

		clients_hash[i] = NULL;
	}
}









