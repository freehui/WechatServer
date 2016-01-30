#include <stdio.h>  
#include <sys/socket.h>  
#include <sys/types.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <unistd.h>  
#include <ctype.h>  
#include <strings.h>  
#include <string.h>  
#include <sys/stat.h>  
#include <pthread.h>  
#include <sys/wait.h>  
#include <stdlib.h>  
#include <time.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include "mysql.h"
#include "json.h"
#include "user_actions.h"
#include "online.h"
#include "client.h"
#include "push.h"
#include "CONFIG.h"

#define MAXBUF 1024
#define MAXEPOLLSIZE 10000

#define error_die(sc)	do {	\
						perror(sc);	\
						exit(1);	\
						} while(0)

MYSQL *m;

struct format{

	int type;
	int length;
	char data[0];
};

/*
 * 建立 TCP 服务
*/
int tcp_startup(int port)  
{  
    int tcp_sock = 0;  
    struct sockaddr_in name;  
	int r;
	
    tcp_sock = socket(PF_INET, SOCK_STREAM, 0);  
    if (tcp_sock == -1)  
        error_die("socket");  
	
	if (setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &r, sizeof(r)) < 0)
		error_die("setsockopet error\n");
	
    memset(&name, 0, sizeof(name));  
    name.sin_family = AF_INET;  
	name.sin_port = htons(port);  
	name.sin_addr.s_addr = htonl(INADDR_ANY);  
	
    if (bind(tcp_sock, (struct sockaddr *)&name, sizeof(name)) < 0)  
        error_die("bind");  

    if (listen(tcp_sock, 5) < 0)  
        error_die("listen");  

    return(tcp_sock);  
}  


void init_databases(void){

	mysql_query(m,"create database wechat");
	mysql_change_user(m,DATABASE_USER,DATABASE_PASSWORD,DATABASE_NAME);
	mysql_query(m,"create table wechat_user(" 	\
				   "id int not null primary key auto_increment," \
 				   "name char(20) not null,"	\
	               "sex int(4) not null,"	\
	               "address char(20) not null,"	\
	               "phonenumber varchar(20) not null,"	\
	               "password varchar(20) not null)");

	mysql_query(m,"create table wechat_temp_message(" 	\
				   "id int(4) not null primary key auto_increment," \
 				   "srcname char(20) not null,"	\
	           	   "srcphone char(20) not null,"	\
	               "dstname char(20) not null,"	\
				   "dstphone char(20) not null,"	\
	               "value longtext not null)");	

	mysql_query(m,"create table wechat_addfriend_message(" 	\
				   "id int(4) not null primary key auto_increment," \
	               "srcphone char(20) not null,"	\
	               "srcname char(20) not null,"	\
	               "dstphone char(20) not null,"	\
	               "value longtext not null)");		

	mysql_query(m,"create table wechat_friend(" 	\
				   "id int(4) not null primary key auto_increment," \
	               "aphone char(20) not null,"	\
	               "aname char(20) not null,"	\
	               "bname char(20) not null,"	\
	               "bphone char(20) not null)");			
}

/*
 * 连接 mysql 并得到其句柄
*/
void init_mysql(void){
	
	if (!(m = mysql_init(NULL)))
		error_die("连接 Mysql 失败，请检查 Mysql 是否正确安装\n"); 

	if (!mysql_real_connect(
		m,DATABASE_IP,DATABASE_USER,DATABASE_PASSWORD,DATABASE_NAME,0,NULL,0)){

		if (mysql_real_connect(m,DATABASE_IP,DATABASE_USER,DATABASE_PASSWORD,NULL,0,NULL,0)){

			printf("不存在 '%s' 数据库,程序将会自动创建该数据库\n:",DATABASE_NAME);
			init_databases();
		}else 
			error_die("连接数据库失败,请检查帐号密码是否正确\n"); 
	}	
}

/*
 * 设置一个 sock 为非阻塞，这样就不会因为读不够数据而阻塞了（因为我们需要用一个缓冲区读取数据）。
*/
void setnonblocking(int sock){
	
	int opts = fcntl(sock,F_GETFL);
	
    if(opts < 0)
		error_die("fcntl(sock,GETFL)");
	
	opts |= O_NONBLOCK;
	
    if(fcntl(sock,F_SETFL,opts)<0)
		error_die("fcntl(sock,SETFL,opts)");
}


int nfds;
int curfds;
int kdpfd;
int udpfd;
int tcp_server_sock = -1;  
int udp_server_sock = -1;  
struct epoll_event events[MAXEPOLLSIZE];
struct epoll_event ev;

/*
 * 断开一个连接
 */
void close_connect(int client){
 
	epoll_ctl(kdpfd,EPOLL_CTL_DEL,client,&ev);
	
	

	Client c = find_client(client);
	if (c != NULL){
		
		push_offline_msg(c->user.phone);
		free_push_client(c->user.phone);
		free_client(client);
	}
	
	close(client);
	curfds--;
}


action_func action_list[] = {
	
	useraction_register,
	useraction_login,
	user_actions_query_unreadmessage,
	user_actions_getmessage,
	user_actions_addfriend,
	
	user_actions_select_addfriend,
	user_actions_okorno,
	user_actions_getfriends,
	user_actions_sendmsg_invalid,
	user_actions_update_image,
	
	user_actions_getimage,
	user_actions_deletefriend,

	connect_push_server
};

/*
 * 处理数据
*/
int handle(int client,void *buf){

	int ret = 0;
	struct online_user *user;
	struct format *data = (struct format *)buf;

	if (data->length == 0)
		return;
	if (data->type > (sizeof(action_list) / 4) || data->type < 0)
		return;

	data->data[data->length] = '\0';
	user = find_user(client);

	if (user == NULL || user->handler == NULL)
		ret = action_list[data->type - 1](client,data->data);

	if (user != NULL && user->handler != NULL)
		ret = user->handler(client,data->data);

	if (ret < 0)  
		close_connect(client);

	return ret;
}

void ctrl_c_handle(int s){  
    
	close_all_connect();
}  

void register_ctrl_c(void){

   struct sigaction sigIntHandler;  
  
   sigIntHandler.sa_handler = ctrl_c_handle;  
   sigemptyset(&sigIntHandler.sa_mask);  
   sigIntHandler.sa_flags = 0;  
  
   sigaction(SIGINT, &sigIntHandler, NULL);  
}

int main(void){
	
	int i;
	int ifd;
    int len;
	int client_sock;
	struct online_user *user;
	Client client;
	struct sockaddr_in client_name;  
	int client_name_len = sizeof(client_name);  
#define MAX_BUFSIZE	0x100000
	char buf[MAX_BUFSIZE];
	struct format *package = (struct format *)buf;

    tcp_server_sock = tcp_startup(SERVER_TCP_PORT);  
	
	init_mysql();
	register_ctrl_c();

	kdpfd = epoll_create(MAXEPOLLSIZE);

	/* 设置要监听的事件。EPOLLIN 表示可读，EPOLLET则指定电平触发 */
    ev.events = EPOLLIN;
	ev.data.fd = tcp_server_sock;
	
	if (epoll_ctl(kdpfd, EPOLL_CTL_ADD, tcp_server_sock, &ev) < 0)
		/* 把 tcp sock 加入监听集合，以便接收连接请求 */		
		error_die("tcp epoll_ctl\n");

	/* 记录监听的文件数，同时他也是 epoll_wait 的第三个参数。
	   因为内核需要知道 events 数组的有效数据有多长 */
	curfds = 1;
	
	for (;;){

		memset(buf,0,sizeof(buf));
		printf("waiting...\n");
		
        /* 等待有事件发生。该函数的返回值存放在 nfds 和 events 内 */
        nfds = epoll_wait(kdpfd, events, curfds, -1);	
		
		if (nfds < 0)
			error_die("epoll_wait");
		
		for (i = 0;i < nfds;i++){
			
			ifd = events[i].data.fd;
			
			if (ifd == tcp_server_sock){
				//新的 TCP 连接请求到来
				
				if (!(events[i].events & EPOLLIN))
					error_die("failed to event is not EPOLLIN\n");
				
				client_sock = accept(tcp_server_sock,(struct sockaddr *)&client_name,&client_name_len); 
				setnonblocking(client_sock);

				ev.events  = EPOLLIN;
				ev.data.fd = client_sock;
                if (epoll_ctl(kdpfd, EPOLL_CTL_ADD, client_sock, &ev) < 0)	
                    error_die("epoll_ctl");
				
				curfds++;
				
				client = alloc_client(client_sock);
				add_client(client);
				
			}else if ((events[i].events & EPOLLIN)){
				/* 客户端有数据发来（POLLIN）或者发生 POLLHUP/POLLERR（这两个事件系统会自动监听） */
				
				client = find_client(ifd);

				if (client != NULL && client->handler != NULL){
					//已经有了处理函数，直接调用这个函数来处理		
					client->handler(ifd,client->private);
				}else {
					//默认的处理方法

					//读入数据包头部。
					len = read(ifd,package,(sizeof(struct format)-sizeof(package->data)));

					if (len <= 0){
						//对方断线。
						close_connect(ifd);
						continue;					
					}
				
					if (package->length >= MAX_BUFSIZE || package->length <= 0){

						while(read(ifd,package->data,4) > 0);
						continue;
					}
				
					if ((len = read(ifd,package->data,package->length) <= 0)){
						//读入数据包内容

						close_connect(ifd);
						continue;
					}

					// 处理数据
					if (handle(ifd,package) == NEED_WRITE){
					
						ev.events = EPOLLOUT;
						ev.data.fd = ifd;
						epoll_ctl(kdpfd,EPOLL_CTL_MOD,ifd,&ev);
					}
				}

			}else if(events[i].events & EPOLLOUT){
				// 有 fd 可写

				client = find_client(ifd);		
				if (client == NULL || client->handler == NULL)
					//必须指定由哪个函数来执行写操作，如果没有指定，那么就报错
					close_connect(ifd);
				else 
					if (client->handler(ifd,(char *)client->private) == NEED_READ){

						ev.events = EPOLLIN;
						ev.data.fd = ifd;
						epoll_ctl(kdpfd,EPOLL_CTL_MOD,ifd,&ev);

						client->handler = NULL;
					}

			}else {

				close_connect(ifd);
			}
		}
	}
 	
	close(tcp_server_sock);
	close(kdpfd);
    return(0);  
}


