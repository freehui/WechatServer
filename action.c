#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "json.h"
#include "user_actions.h"
#include "mysql.h"
#include "online.h"
#include "client.h"
#include "push.h"

extern MYSQL *m;

int read_file(int client,char *value){
	
	struct upload *up = (struct upload *)value;
	Client c;
	char buf[600];
	int len;

	len = read(client,buf,500);
	write(up->fd,buf,len);
	up->index += len;

	if (up->index >= up->total){

		c = find_client(client);
		if (c == NULL)
			return -1;

		close(up->fd); 
		free(up); 
		c->private = NULL;
		c->handler = NULL;

		return NEED_READ;
	}	

	return 0;
}

/*
 * 更新头像
 */
int user_actions_update_image(int client,char *value){
	
	char path[100];
	JSON_obj *obj;
	int imagesize;
	struct upload *up;
	Client c;

	c = find_client(client);
	if (c == NULL)
		return -1;
	
	obj = parser(value);

	up = malloc(sizeof(struct upload));
	up->index = 0;
	up->total = atoi(getvalue_bykey(obj,"imglen"));
	
	sprintf(path,"%s.jpg",getvalue_bykey(obj,"username"));
	remove(path);
	
	up->fd = open(path,O_WRONLY|O_CREAT,0777);

	c->private = up;
	c->handler = read_file;

	free_json(obj);

	return 0;
}


int user_actions_getimage(int client,char *phone){
	

	int ret[1];
	
	struct online_user *user = find_phone (phone);
	char buf[100];
	struct stat filestat;
	sprintf(buf,"%s.jpg",phone);
	stat(buf, &filestat);
	int fd = open(buf,O_RDONLY);
	int len = filestat.st_size;

	if (len == 0){
		//文件长度为 0，就说明这个文件本不存在。返回0.
		ret[0] = 0;
		write(client,ret,sizeof(int));
		return 0;
	}
	
	char *filebuf = malloc(len);

	//写出文件长度
	write(client,&len,sizeof(int));
	
	//把文件读到缓冲区
	read(fd,filebuf,len);
	
	//写出文件
	write(client,filebuf,len);

	free(filebuf);
	
	return 0;
}

/*
 * 删除好友
 */
int user_actions_deletefriend(int client,char *value){

	char buf[100];
	char *phone = value;
	
	struct online_user *ou = find_user (client);
	if (ou == NULL)
		return -1;
	 
	sprintf(buf,"delete from wechat_friend where (aphone = %s and bphone = %s) or (bphone = %s and aphone = %s)",
		    ou->phone,phone,ou->phone,phone);	

	mysql_query(m,buf);
}	


/*
 * 注册
 */
int useraction_register(int cli_ent,char *json_text){

	char buf[500];
	int ret[1];
	MYSQL_RES *m_res;
	JSON_obj *obj = parser(json_text);
	char *username = getvalue_bykey(obj,"username");
	char *phonenumber = getvalue_bykey(obj,"phonenumber");
	char *password = getvalue_bykey(obj,"password");

	sprintf(buf,"select id from wechat_user where phonenumber = '%s' ",\
	        phonenumber);
	mysql_query(m,buf);
	m_res = mysql_use_result(m);

	if (m_res == NULL){

		ret[0] = 1;
		write(cli_ent,ret,sizeof(ret));
		return;
	}

	memset(buf,0,sizeof(buf));
	sprintf(buf,"insert into wechat_user(name,phonenumber,password)"	\
	        	"value('%s','%s','%s')",username,phonenumber,password);
	mysql_query(m,buf);

	free_json(obj);
	ret[0] = 2;
	write(cli_ent,ret,sizeof(ret));

	return 0;
}

/*
 * 登陆
 */
int useraction_login(int cli_ent,char *json_text){
	
	MYSQL_RES *m_res;
	MYSQL_ROW m_row;
	char buf[100];
	int ret[1];
	int func_ret = 0;
	struct online_user *user;
	JSON_obj *obj = parser(json_text);

	
	char *phonenumber = getvalue_bykey(obj,"phonenumber");
	char *password = getvalue_bykey(obj,"password");	

	if (user = find_phone (phonenumber)){

		close_connect(user->sock);
	}

	sprintf(buf,"select name from wechat_user where phonenumber = '%s' and password = '%s'",\
	        phonenumber,password);

	mysql_query(m,buf);
	m_res = mysql_use_result(m);
	
	if (m_res == NULL){
		//未注册
		ret[0] = 1; 
		write(cli_ent,ret,sizeof(ret));
	}else {

		if (m_row = mysql_fetch_row(m_res)){
			/* 读出一行即可，因为不允许重名的用户名，所以 name=username 的结果只能有一个 */
			/* 如果真的读出了，那就表示这个用户名已经被注册了，可以登录 */

			struct online_user *u = &find_client(cli_ent)->user;
			u->sock = cli_ent;
			strcpy(u->phone,phonenumber);
			strcpy(u->name,m_row[0]);

			online_user(u);
			ret[0] = 2;
			write(cli_ent,ret,sizeof(ret));
		}else {
			// 未注册
			ret[0] = 1; 
			write(cli_ent,ret,sizeof(ret));
		}
		mysql_free_result(m_res);
	}

	free_json(obj);
	return func_ret;
}

/*
 * 查询所有未读的讯息
 */
int user_actions_query_unreadmessage(int cli_ent,char *json_text){

	int i;
	int ret[1];
	MYSQL_RES *m_res;
	MYSQL_ROW m_row;
	char buf[100];
	struct online_user *ou = find_user (cli_ent);
	if (ou == NULL)
		return -1;

	sprintf(buf,
	        "select srcname,srcphone,value from wechat_temp_message where dstphone = \'%s\'",
	        ou->phone);
	
	mysql_query(m,buf);
	m_res = mysql_store_result(m);
	
	if (m_res == NULL){

		ret[0] = 0;
		write(cli_ent,ret,sizeof(ret));
		return 0;
	}else {

		int count = mysql_num_rows(m_res);

		if (count > 0){
			
			struct {

				int length;
				char buf[0];
			} *out = malloc((count + 1) * 500);

			out->buf[0] = '[';
			out->length = 1;

			for (i = 0;i < count;){

				m_row = mysql_fetch_row(m_res);
				out->length += sprintf(&out->buf[out->length],
				        			   "{\"name\":\"%s\",\"phone\":\"%s\",\"value\":\"%s\"}",
				        			   m_row[0],m_row[1],m_row[2]);
				if ((++i) < count)
					out->buf[out->length++] = ',';
			}
			                
			out->buf[out->length++] = ']';
			out->buf[out->length] = '\0';
			write(cli_ent,out,out->length + sizeof(int));
			free(out);
		}else {

			ret[0] = 0;
			write(cli_ent,ret,sizeof(ret));
		}
	}
	
	return 0;
}

/*
 * 获取好友列表
 */
int user_actions_getfriends(int cli_ent,char *json_text){

	int i;
	MYSQL_RES *m_res;
	MYSQL_ROW m_row;
	char buf[300];
	int ret[1];
	struct online_user *ou = find_user (cli_ent);

	if (ou == NULL)
		return -1;	

	sprintf(buf,"select aphone,bphone,aname,bname from wechat_friend where aphone = '%s' or bphone = '%s'",
		    ou->phone,ou->phone);	

	mysql_query(m,buf);
	m_res = mysql_store_result(m);

	if (m_res == NULL){

		ret[0] = 0;
		write(cli_ent,ret,sizeof(ret));
		return 0;
	}else {
		
		int count = mysql_num_rows(m_res);
		
		if (count > 0){

			struct online_user *friend;
			char *friend_phone = NULL;
			char *friend_ip = NULL; 
			char *friend_name = NULL; 
			struct {

				int length;
				char buf[0];
			} *out = malloc((count + 1) * 80);

			if (out == NULL){

				ret[0] = 0;
				write(cli_ent,ret,sizeof(ret));				
				return 0;
			}

			out->buf[0] = '[';
			out->length = 1;

			for (i = 0;i < count;){

				m_row = mysql_fetch_row(m_res);

				if (strcmp(m_row[0],ou->phone) == 0){
					/* 取出 friend 的手机号和用户名 */
					friend_phone = m_row[1];
					friend_name = m_row[3];
				}else{

					friend_phone = m_row[0];
					friend_name = m_row[2];					
				}

				friend = find_phone (friend_phone);
				if (friend){

					friend_ip = friend->ip;
				}else{

					friend_ip = "0";
				}

				out->length += sprintf(&out->buf[out->length],
				        			   "{\"name\":\"%s\",\"phone\":\"%s\",\"ip\":\"%s\"}",
				        			   friend_name,friend_phone,friend_ip);
				if ((++i) < count)
					out->buf[out->length++] = ',';
			}
			                
			out->buf[out->length++] = ']';
			out->buf[out->length] = '\0';
			write(cli_ent,out,out->length + sizeof(int));

			free(out);
	
		}else {

			ret[0] = 0;
			write(cli_ent,ret,sizeof(ret));
		}
	}

	return 0;
}

/*
 * 得到未处理的好友请求
 */
int user_actions_select_addfriend(int cli_ent,char *json_text){

	int i;
	int ret[1];
	MYSQL_RES *m_res;
	MYSQL_ROW m_row;
	char buf[300];
	struct online_user *ou = find_user (cli_ent);
	if (ou == NULL)
		return -1;
	
	sprintf(buf,
	        "select srcname,srcphone,value from wechat_addfriend_message where dstphone = \'%s\'",
	        ou->phone);
	
	mysql_query(m,buf);
	m_res = mysql_store_result(m);

	if (m_res == NULL){

		ret[0] = 0;
		write(cli_ent,ret,sizeof(ret));
		return 0;
	}else {

		int count = mysql_num_rows(m_res);

		if (count > 0){
			
			struct {

				int length;
				char buf[0];
			} *out = malloc((count + 1) * 500);

			out->buf[0] = '[';
			out->length = 1;

			for (i = 0;i < count;){

				m_row = mysql_fetch_row(m_res);
				out->length += sprintf(&out->buf[out->length],
				        			   "{\"srcname\":\"%s\",\"phone\":\"%s\",\"value\":\"%s\"}",
				        			   m_row[0],m_row[1],m_row[2]);
				if ((++i) < count)
					out->buf[out->length++] = ',';
			}
			                
			out->buf[out->length++] = ']';
			out->buf[out->length] = '\0';
			write(cli_ent,out,out->length + sizeof(int));
			free(out);
	
		}else {

			ret[0] = 0;
			write(cli_ent,ret,sizeof(ret));
			return 0;
		}
	}
	
	return 0;
}


/*
 * 得到一个用户的信息
 */
int user_actions_getmessage(int cli_ent,char *phone){

	MYSQL_RES *m_res;
	MYSQL_ROW m_row;
	char buf[100];
	int ret[1];

	sprintf(buf,"select name,phonenumber from wechat_user where phonenumber = '%s'",phone);

	mysql_query(m,buf);
	m_res = mysql_use_result(m);

	if (m_res == NULL){

		ret[0] = 0;
		write(cli_ent,ret,sizeof(ret));
	}else {

		if (m_row = mysql_fetch_row(m_res)){
			/* 读出一行即可，因为不允许重名的用户名，所以 name=username 的结果只能有一个 */
			/* 如果真的读出了，那就表示这个用户名已经被注册了 */

			struct {

				int length;
				char buf[0];
			} *out = malloc(100);

			out->length = sprintf(out->buf,"{\"name\":\"%s\",\"phone\":\"%s\"}",
			                      m_row[0],m_row[1]);
			out->buf[out->length] = '\0';
			write(cli_ent,out,out->length + sizeof(int));

			free(out);
		}else {

			ret[0] = 1;
			write(cli_ent,ret,sizeof(ret));
		}
		mysql_free_result(m_res);
	}

	return 0;
}

/*
 * 发送好友申请
 */
int user_actions_addfriend(int cli_ent,char *json_text){

	MYSQL_RES *m_res;
	MYSQL_ROW m_row;
	char buf[300];
	JSON_obj *obj = parser(json_text);
	char *dstphone = getvalue_bykey(obj,"phone");
	char *srcname = getvalue_bykey(obj,"name");
	char *v = getvalue_bykey(obj,"value");
	int ret[1];

	struct online_user *cur = find_user(cli_ent); 
	struct online_user *dst; 
	
	if (cur == NULL)
		return -1;

	sprintf(buf,
	        "select id from wechat_addfriend_message where dstphone = %s and srcphone = %s"
	        ,dstphone,cur->phone);

	mysql_query(m,buf); 
	m_res = mysql_use_result(m); 

	if (m_res != 0){
		//没有在数据库中找到

		m_row = mysql_fetch_row(m_res); 
		mysql_free_result(m_res);
	
		if (m_row){
			/* 已经发送过添加好友的请求 */

			//return 0;
			goto out;
		}
	}

	memset(buf,0,sizeof(buf));
	
	dst = find_phone(dstphone);

	sprintf(buf,"insert into wechat_addfriend_message(srcname,srcphone,dstphone,value) \
					 values(\"%s\",\"%s\",\"%s\",\"%s\")",srcname,cur->phone,dstphone,v);

	mysql_query(m,buf);

	if (dst){
			/* dst 不为空，说明目标在线，直接发送请求 */
		struct format {

			int type;
			int valuelength;
			char  buf[0];
		} *d = (struct format *)buf;		
		d->type = PUSH_ADDREQUEST;

		d->valuelength = 
			sprintf(d->buf,
			        "{\"name\":\"%s\",\"phone\":\"%s\",\"value\":\"%s\"}",
			        	cur->name,cur->phone,v);

		push_msg(dstphone,buf,d->valuelength + sizeof(int)*2);		
	}

out:
	free_json(obj);
	return 0;
}


/*
 * 接受或拒绝好友申请
 */
int user_actions_okorno(int cli_ent,char *json_text){

	int i;
	MYSQL_RES *m_res;
	MYSQL_ROW m_row;
	char buf[300];
	JSON_obj *obj;
	char *phone;
	char *name;
	struct online_user *ou = find_user (cli_ent);
	struct online_user *dst;

	if (ou == NULL)
		return -1;
	
	obj = parser(json_text);
	phone = getvalue_bykey(obj,"phone");
	name = getvalue_bykey(obj,"name");
	
	//删除好友申请记录
	sprintf(buf,"delete from wechat_addfriend_message where dstphone = %s and srcphone = %s",
		    ou->phone,phone);	
	mysql_query(m,buf);

	char *type = getvalue_bykey(obj,"type");
	
	if (strcmp(type,"0") == 0){
		/* 拒绝，那么什么都不用做 */

	}else {
		/* 接受，那么写入好友信息 */

		sprintf(buf,
		        "insert into wechat_friend(aphone,bphone,aname,bname) values(\"%s\",\"%s\",\"%s\",\"%s\")",
				ou->phone,phone,ou->name,name);

		mysql_query(m,buf);

		dst = find_phone (phone);
		//如果被接受者不在线，那么就不通知他。
		if (dst == NULL)
			goto out;
		
		struct format {

			int type;
			int namelength;
			char  buf[0];
		} *d = (struct format *)buf;
		d->type = PUSH_ADDNOTIFI;
		d->namelength = sprintf(d->buf,
		                        "{\"name\":\"%s\",\"phone\":\"%s\",\"ip\":\"%s\"}",
		                        ou->name,ou->phone,ou->ip);
		push_msg(phone,buf,d->namelength + sizeof(int)*2);	
	}

out:
	free_json(obj);
	return 0;
}

/*
 * P2P 发送消息失败时，本函数来进行处理。
 * 他通知对端进行打洞，并获取最新 IP，同时把消息发送过去。
 * 
 * p.s. 由于某些原因，去掉 P2P 功能，故本函数多余部分去掉，以后考虑加回。
 */
int user_actions_sendmsg_invalid(int cli_sock,char *json_text){

	int len = 0;
	JSON_obj *obj = parser(json_text);
	char *dstphone = getvalue_bykey(obj,"dstphone");
	char *dstname = getvalue_bykey(obj,"dstname");
	char *v = getvalue_bykey(obj,"value");	
	char buf[500];
	struct online_user *cur = find_user(cli_sock); 
	struct online_user *dst = find_phone(dstphone); 
	
	if (dst == NULL){
		//如果接收者不在线，那么需要把数据写到数据库里。当接收者上线时，就能接收到
		
		sprintf(buf,
				"insert into wechat_temp_message(srcname,srcphone,dstname,dstphone,value) values('%s','%s','%s','%s','%s')",
					cur->name,cur->phone,dstname,dstphone,v); 
		printf("%s\n",buf);
		mysql_query(m,buf);
		goto out;
	}

	struct format {

		int type;
		int len;
		char buf[0];
	} *d = (struct format *)buf;
	//发送消息
	d->type = PUSH_CHATMESSAGE;
	d->len = sprintf(d->buf,
		            "{\"name\":\"%s\",\"phone\":\"%s\",\"value\":\"%s\",\"ip\":\"%s\"}",
		            cur->name,cur->phone,v,cur->ip);
	push_msg(dstphone,buf,d->len + sizeof(int)*2);	

out:
	free_json(obj);
	return 0;
}









