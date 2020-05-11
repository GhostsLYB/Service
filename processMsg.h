#ifndef _PROCESSMSG
#define _PROCESSMSG

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include "mysqlHelp.h"
#include "data.h"
#include <pthread.h>

void addPacketLen(char ** msg);

/*重新实现write函数为writen函数 实现发送固定长度的数据*/
ssize_t writen(int fd, const void *buf, size_t count);

void fun(char** buf);

void outInfo(char * buf);

void* sendFile(void* args);

/* 注册处理函数*/
/* *msg数据格式：[长度][数据段][长度][数据段]。。。。。。*/
/* 注册成功将yes存入，失败存入no*/
void processRegister(char ** msg);

void processClientExit(int *send_fd,map<string,int> * userSocketMap);

/* 登陆请求函数 *msg : [长度][数据段][长度][数据段]。。。。。。 */
void processLogin(char ** msg, int *send_fd, map<string,int> * userSocketMap);

//消息转发处理函数
//*msg格式：[类型][接收用户名长][接收用户名][消息长度][消息]
void processRelay(char ** msg,int * send_fd, map<string,int> * userSocketMap);

//消息处理函数
//*msg数据格式：[类型][长度][数据段][长度][数据段]。。。。。。
void * process(struct MsgProcessPacket * args);

void sendSyncFile(const char* userName, const int fd);

void processRecvFile(char ** msg, int client_fd);

void processSendFile(char ** msg, int client_fd);

void processModifyInfo(char ** msg);

void processDeleteFriend(char ** msg, int *send_fd, map<string,int> * userSocketMap);

void processAddFriend(char **msg, int *send_fd, map<string,int> *userSocketMap);

void processAddFriendResponse(char **msg, int *send_fd, map<string,int> *userSocketMap);

#endif
