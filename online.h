#ifndef _INCLUDE_ONLINE_H
#define _INCLUDE_ONLINE_H

#include "user_actions.h"

struct online_phone;


/*
 * 客户端下载用的结构体
 */
typedef struct Download {
	char *start; //数据存放的开始地址
	int total; //总传毒长度
	int index; //当前传输的数据长度
} Download;

/*
 * 客户端上传用的结构体
 */
struct upload {
	
	int fd;  //写入的文件
	int total; //总传输长度
	int index; //当前传输长度
};


/* 描述一个在线用户 */
struct online_user{

	int sock;/* 该用户的 socket */
	int lasttime;/* 该用户上次发送心跳包的时间 */
	char phone[12];/* 该用户的手机号 */
	char name[20];/* 该用户的手机号 */
	char ip[20]; /* 该用户的 ip 地址 */
	action_func handler;
	void *private;
	struct online_phone *pptr;/* 指向这个 online_user 对应的 online_phone */
	struct online_user *next;
	struct online_user *prev;
};

/* struct online_user 只能通过 socket fd 来索引，本结构体可以通过 phone 索引 */
struct online_phone{

	struct online_user *user;/* online_phone 只是为了方便用手机号搜索 online_user，所以他也要指向他 */
	struct online_phone *next;
	struct online_phone *prev;	
};

//extern struct online_user *online_user(int,char *,char *);
extern struct online_user *online_user(struct online_user *new);
extern void offline_user(int sock);
extern struct online_user * find_user(int sock);
extern struct online_user * find_phone(char *phone);

#endif
