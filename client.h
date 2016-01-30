#ifndef _CLIENT_H
#define _CLIENT_H

#include "online.h"

typedef void (*handler_func)(int client,void *);

typedef struct Client {
	int sock;
	action_func handler; //处理当前客户端请求的函数
	struct online_user user; //该用户的在线状态
	void *private; //handle_func() 使用的私有数据
	struct Client *next;
	struct Client *prev;
} *Client;

#define get_client(src)  ((Client)((int)src - (int)&(((Client )0)->user)))

extern Client alloc_client(int sock);
extern void add_client(Client new);
extern Client find_client(int sock);
extern void free_client(int sock);
extern Client del_client(int sock);
extern void close_connect(int client);
extern void close_all_connect(void);

#endif 
