#ifndef USER_ACTINS_H
#define USER_ACTINS_H

/* 注册 */
#define USER_ACTIONS_REGISTER (1)
/* 登陆 */
#define USER_ACTIONS_LOGIN    (2)
/* 查看所有未接受的消息 */
#define USER_ACTIONS_SELECT    (3)
/* 查询用户信息 */
#define USER_ACTIONS_MESSAGE  (4)
/* 添加好友 */
#define USER_ACTIONS_ADDFRIEND  (5)
/* 查询好友申请请求 */
#define USER_ACTIONS_SELECT_ADDFRIEND_MESSAGE  (6)
/* 接受或拒绝好友申请 */
#define USER_ACTIONS_OKORNO  (7)
/* 获得好友列表 */
#define USER_ACTIONS_FRIENDS  (8)
/* 处理 P2P 消息发送失败 */
#define USER_ACTIONS_SENDMSG_INVALID  (9)
/* 更新头像 */
#define USER_ACTIONS_UPDATE_IMAGE (10)
/* 获得一个用户的头像 */
#define USER_ACTIONS_GET_IMAGE (11)
/* 删除好友 */
#define USER_ACTIONS_DEL_FRIEND (12)

/* 推送 Socket */
#define PUSH_SOCKET (13)

/* 推送：对方接受好友请求 */
#define PUSH_ADDNOTIFI 2	
/* 推送：对方发来好友请求 */
#define PUSH_ADDREQUEST 3
/* 推送：对方发来消息 */
#define PUSH_CHATMESSAGE 8

/* 需要写 */
#define NEED_WRITE (100)
/* 需要读 */
#define NEED_READ (99)

typedef int (*action_func)(int,char *data);

extern int useraction_register(int,char *data);
extern int useraction_login(int,char *data);
extern int user_actions_query_unreadmessage(int,char *data);
extern int user_actions_getmessage(int,char *data);
extern int user_actions_addfriend(int,char *data);
extern int user_actions_select_addfriend(int,char *data);
extern int user_actions_okorno(int,char *data);
extern int user_actions_getfriends(int,char *data);
extern int user_actions_sendmsg_invalid(int,char *data);
extern int user_actions_update_image(int,char *data);
extern int user_actions_getimage(int,char *data);
extern int user_actions_deletefriend(int,char *data);

#endif
