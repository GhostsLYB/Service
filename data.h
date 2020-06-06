#ifndef _DATA
#define _DATA

#include <iostream>
#include <stdio.h>
#include <map>
#include <pthread.h>
#include <string>

using namespace std;

/*用于服务器接收标准输入的命令，查询服务器的连接情况和表的数据*/
struct CheckServiceDataPacket{
	pthread_mutex_t  *pMutex;
	map<string, int> *pMap;
	bool 		 *isExit;/*可以通过命令让服务器退出*/
};

/*消息处理结构体，用于处理不同消息，作为process函数的参数*/
struct MsgProcessPacket
{
	pthread_mutex_t  *pMutex;
	char 		 *msg;
	int 		 send_fd;
	map<string, int> *userSocketMap;
	int 		 *exitSocket;
};

/*处理客户端消息结构体，用户处理客户端套接字有数据可读时作为线程函数参数*/
struct ProcessClientMsgPacket 
{
	int 		epfd;           /*主函数中的epfd*/
	int 		client_fd;      /*与客户端通信的套接字文件描述符*/
	map<string,int> * pMap;   	/*登陆用户的用户名与连接套接字的map指针*/
	pthread_mutex_t * pMutex;	/*主函数中的互斥锁*/
	int 		*exitSocket;	/**/
};

#endif 
