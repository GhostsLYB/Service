#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <map>
#include "processMsg.h"
#include "data.h"
using namespace std;

#define MAXSIZE  1024
#define SENDSIZE 4296

#define ERR_EXIT(m) \
	do \
	{ \
		perror(m); \
		exit(EXIT_FAILURE); \
	} while(0)

void *processClientMsg(void *);
void * checkServiceData(void *args);

ssize_t readn(int fd, void *buf, size_t count)
{
	ssize_t nread;	//已经读取的字节数
	size_t nleft = count;	//剩余字节数
	char* bufp = (char*)buf;//定义指向存储缓冲区的指针
	
	while(nleft > 0)
	{	
		if((nread = read(fd, bufp, nleft)) < 0)
		{
			if(errno == EINTR)
				continue;
			return -1;
		}
		else if(nread == 0)
			return count - nleft;
		bufp += nread;
		nleft -= nread;
	}
	return count;
}

void handle_sigchld(int sig)
{
	//wait(NULL);
	//等待所有子进程退出，没有子进程退出是返回
	while(waitpid(-1, NULL, WNOHANG) > 0) ;//如果有子进程退出，循环waitpid
}

//服务端程序
int main(void)
{
	signal(SIGPIPE, SIG_IGN);
	//signal(SIGCHLD, SIG_IGN);//忽略SIGCHLD信号 避免僵进程	
	signal(SIGCHLD,handle_sigchld);//捕捉SIGCHLD,用handle_sigchld函数处理

	int listenfd;
	//创建一个套接字
	if((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		ERR_EXIT("socket");
	
	//初始化一个地址
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(5188);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);//本机上的任意地址
	//servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");//本机地址
	
	//设置SO_REUSEADDR选项使服务器在TIME_WITE状态时就可以重新启动（地址重复利用）
	int on = 1;
	if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
		ERR_EXIT("setsockopt");	
	
	//将套接字与地址绑定（本机所有地址或只有本机地址）
	if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
		ERR_EXIT("bind");
	
	//将套接字设置为被动套接字，用于接受连接，最大连接数为SOMAXCONN	
	if(listen(listenfd, SOMAXCONN) < 0)
		ERR_EXIT("listen");

	//初始化一个地址，用于存储连接的客户端地址
	struct sockaddr_in peeraddr;
	int conn;
	socklen_t peerlen = sizeof(peeraddr);
	
	//在单进程中使用epoll维护多个套接口（包括监听套接口和已连接套接口）
	
	struct epoll_event ev, events[MAXSIZE] = {0};
	ev.events = EPOLLIN;
	ev.data.fd = listenfd;

	int epfd = epoll_create(MAXSIZE);
	if(epfd == -1)
		ERR_EXIT("epoll create");
	
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
	if(ret == -1)
		ERR_EXIT("epoll ctl");
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = 0;
	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, 0, &ev);
	if(ret == -1)
		ERR_EXIT("epoll ctl");

	map<string, int> userSocketMap;
	pthread_mutex_t mutex;
	ret = pthread_mutex_init(&mutex,NULL);	//or mutex = PTHREAD_MUTEX_INITIALIZER
	
	if(ret == 0)
		printf("check service data process successed\n");
	else
		printf("check service data process fail\n");
	bool isExit = false;
	while(1)
	{
		if(isExit)
			break;
		int num = epoll_wait(epfd, events, MAXSIZE, -1);
		if(num == -1)
		{
			if(errno != EINTR )
				ERR_EXIT("epoll_wait");
		}
		if(num == 0)
			continue;
		for(int i = 0; i < num ; i++)
		{
			if(events[i].data.fd == 0)
			{
				struct CheckServiceDataPacket checkPacket;
				memset(&checkPacket, 0, sizeof(checkPacket));
				checkPacket.pMutex = &mutex;
				checkPacket.pMap = &userSocketMap;
				checkPacket.isExit = &isExit;
				pthread_t checkTid;
				pthread_create(&checkTid, NULL, checkServiceData, &checkPacket);
			}
			else if(events[i].data.fd == listenfd)
			{
				conn = accept(events[i].data.fd, (struct sockaddr*)&peeraddr, &peerlen);
				if(conn < 0)
					ERR_EXIT("accept");
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = conn;
				ret = epoll_ctl(epfd, EPOLL_CTL_ADD, conn, &ev);
				if(ret == -1)
					ERR_EXIT("epoll ctl");
				struct sockaddr_in localaddr;
	                        socklen_t addrlen = sizeof(localaddr);
				if(getsockname(conn, (struct sockaddr*)&localaddr, &addrlen) < 0)
                                      ERR_EXIT("getsockname");
     		                printf("本机IP:%s port:%d\n",inet_ntoa(localaddr.sin_addr), ntohs(localaddr.sin_port));
				printf("对方IP:%s port:%d\n",inet_ntoa(peeraddr.sin_addr), ntohs(peeraddr.sin_port));
			}
			else if (events[i].events & EPOLLIN)		//events is ready read
			{
				//创建线程处理客户端消息（包括对epoll和map的维护）
				pthread_t tid;
				ProcessClientMsgPacket packet;
				memset(&packet, 0, sizeof(packet));
				packet.epfd = epfd;
				packet.client_fd = events[i].data.fd;
				packet.pMap = &userSocketMap;
				packet.pMutex = &mutex;
				ret = pthread_create(&tid, NULL, processClientMsg, &packet);
				if(ret == 0)
					printf("create thread for process client message successed\n");
				else
					printf("create thread fail\n");
			}
		}
	}
	ret = pthread_mutex_destroy(&mutex);
	if(ret == 0)
		printf("destroy mutex successed\n");
	else
		printf("destroy mutex fail\n");
	return 0;
}

//服务器端查看数据的线程函数
void * checkServiceData(void *args)
{
	struct CheckServiceDataPacket * packet = (struct CheckServiceDataPacket*)args;
	pthread_mutex_t * pMutex = packet->pMutex;
	map<string, int> *pMap = packet->pMap;
	bool * isExit = packet->isExit;
	
	char buf[1024] = {0};
	char *pbuf = buf;
	if(NULL != fgets(pbuf, sizeof(buf), stdin))
	{
		int len = strlen(pbuf);
		pbuf[len-1] = '\0';
		if(strcmp(pbuf,"show client") == 0)
		{
			pthread_mutex_lock(pMutex);
			if(pMap->begin() == pMap->end())
			{
				printf("没有已连接的客户端\n");
				memset(pbuf, 0, sizeof(pbuf));
				pthread_mutex_unlock(pMutex);
				return NULL;
			}
			printf("已连接的客户端用户名和套接字如下:\n");
			map<string, int>::iterator iter = pMap->begin();
			printf("+-----------+------+-----------------+-------+\n");
			printf("|  登陆用户 |套接字|   客户端ip地址  |  端口 |\n");
			printf("+-----------+------+-----------------+-------+\n");
			for(; iter != pMap->end(); iter++)
			{
				struct sockaddr_in peeraddr;
                                socklen_t addrlen = sizeof(peeraddr);
                                getpeername(iter->second, (struct sockaddr*)&peeraddr, &addrlen);
				printf("|%10s |%4d  |%16s |%6d |\n",iter->first.c_str(), iter->second, inet_ntoa(peeraddr.sin_addr), ntohs(peeraddr.sin_port));
			}
			printf("+-----------+------+-----------------+-------+\n");
			pthread_mutex_unlock(pMutex);
		}
		else if(strncmp(pbuf, "show table ", 11) == 0)
		{
			char *pName = pbuf + 11;
			printf("%s\n",pName);
			if(!isExistTable(pName))
				printf("表%s不存在\n",pName);
			else
				showTable(pName);
		}
		else if(strncmp(pbuf, "del tmp ", 8) == 0)
		{
			char *pName = pbuf + 8;
			char filePath[50] = {0};
			sprintf(filePath, "/tmp/%s", pName);
			remove(filePath);
		}
		else if(strcmp(pbuf, "help") == 0)
		{
			printf("help        	     : 显示帮助信息\n");
			printf("show client 	     : 显示已登陆的用户和连接的TCP套接字\n");
			printf("show table tableName : 显示表tableNamez中的数据\n");
			printf("del tmp fileName     : 删除文件/tmp/fileName\n");
			printf("exit        	     : 退出服务器程序\n");
		}
		else if(strcmp(pbuf, "exit") == 0)
			*isExit = true;
		else
			printf("命令\"%s\"不正确！\n", pbuf);
	}
	else
		printf("从标准输入流读取数据存错\n");
	return NULL;
}

//处理客户端消息线程的run函数args为processClientMsgPacket *
void * processClientMsg(void * args)
{
	/*获取args参数的数据*/
	int 		epfd 	  = ((struct ProcessClientMsgPacket*)args)->epfd;
	int 		client_fd = ((struct ProcessClientMsgPacket*)args)->client_fd;
	map<string,int> *pMap 	  = ((struct ProcessClientMsgPacket*)args)->pMap;
	pthread_mutex_t *pMutex   = ((struct ProcessClientMsgPacket*)args)->pMutex;


	char recvbuf[SENDSIZE] = {0};
	char temp[1024] = {0};
	char * p = temp;
	pthread_mutex_lock(pMutex);
	readn(client_fd, recvbuf, 4);		/*读取消息长度*/
	int len = atoi(recvbuf);
	printf("recvbuf message length is %d\n",len);
	memset(recvbuf, 0, sizeof(recvbuf));
	int ret = readn(client_fd, recvbuf, len);	/*读取消息*/
	pthread_mutex_unlock(pMutex);
	if(ret == -1)
		ERR_EXIT("readn");
	if(ret == 0)				/*客户端关闭*/
        {
		string exitUserName = "";
		pthread_mutex_lock(pMutex);	/*将userSocketMap中对应的项删除*/
		for(map<string,int>::iterator iter = pMap->begin(); iter != pMap->end();iter++)
		{
			if(iter->second == client_fd)
			{
				exitUserName = iter->first;
				pMap->erase(iter);
				break;
			}
		}
		if(exitUserName != "")		/*保存退出日志（存入用户退出登陆时间）*/
			saveExitLog(exitUserName);
                printf("client closed\n");
		struct epoll_event ev;		/*初始化一个epoll_event*/
		ev.events = EPOLLIN;
		ev.data.fd = client_fd;
		ret = epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, &ev);
                close(client_fd);		/*关闭客户端套接字*/
		pthread_mutex_unlock(pMutex);
                return NULL;
	}
	printf("len = %d recvbuf = [%s]\n",len,recvbuf);
        //outInfo(recvbuf);
	p = recvbuf;
	struct MsgProcessPacket packet;
	memset(&packet, 0, sizeof(packet));
	packet.pMutex = pMutex;
	packet.msg = p;	
	//printf("packet.msg = [%s]\n",packet.msg);
	packet.send_fd = client_fd;
	printf("!!!!!!!!!sendfd = %d !!!!!!!!!!!\n", client_fd);
	packet.userSocketMap = pMap;
	printf("aaaaaaaaaaa\n");
	process(&packet); 
	printf("aaaaaaaaaaa\n");
	return NULL;
}
