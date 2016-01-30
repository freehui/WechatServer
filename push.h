#ifndef _PUSH_H
#define _PUSH_H

extern int connect_push_server (int client,char *value);
extern void free_push_client(char *phone);
extern void push_msg(char *phone,char *msg,int len);
void push_offline_msg(char *phone);

#endif

