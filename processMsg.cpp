#include "processMsg.h"

pthread_mutex_t *pMutex;

void addPacketLen(char ** msg)
{
	char temp[1024] = {0};
	sprintf(temp,"%4d",strlen(*msg));
	strcat(temp,*msg);
	memset(*msg,0,sizeof(*msg));
	strcat(*msg,temp);
}

/*重新实现write函数为writen函数 实现发送固定长度的数据*/
ssize_t writen(int fd, const void *buf, size_t count)
{
        ssize_t nwritten;       //已经发送的字节数
        size_t nleft = count;   //剩余字节数
        char* bufp = (char*)buf;//定义指向存储缓冲区的指针
        while(nleft > 0)
        {       
                if((nwritten = write(fd, bufp, nleft)) < 0)
                {
                        if(errno == EINTR)
                                continue;
                        return -1;
                }
                else if(nwritten == 0)
                        continue;
                bufp += nwritten;
                nleft -= nwritten;
        }
        return count;
}

void fun(char** buf)
{
        int num = 0;
        char temp[1024] = {0};
        strncpy(temp, *buf, 4);
        *buf += 4;
        num = atoi(temp);
        memset(temp, 0, sizeof(temp));
        strncpy(temp, *buf, num);
        *buf += num;
        printf("** %s **\n", temp);
}

void outInfo(char * buf)
{       
        printf("*******************\n");
        int flag;
        char temp[1024] = {0};
        strncpy(temp, buf, 4);
        flag = atoi(temp);
        printf("Message flag = %d\n", flag);
        char * p = buf + 4;
        while(*p != '\0' && *p != '\n')
        {
                fun(&p);
        }
        printf("*******************\n");
}
/* 注册处理函数*/
/* *msg数据格式：[长度][数据段][长度][数据段]。。。。。。*/
/* 注册成功将yes存入，失败存入no*/
void processRegister(char ** msg)
{
	char temp[1024] = {0};
	char * p = *msg;
	int len = 0;
	strncpy(temp, p, 4);
	p = p + 4;
	len = atoi(temp);
	char args[9][50];
	memset(args, 0, sizeof(args));
	strncpy(args[0], p, len);
	p = p + len;
	char * username = temp;
	if(!isInited())
		initMysql();
	int ret = selectUser(args[0]);
	if(ret == 0)		/* 数据库中没有username的记录，可以注册*/
	{
		int i = 1;
		while(*p != '\0' && *p != '\n' && i < 9)		/*获取数据到args中*/
		{
			memset(temp, 0, sizeof(temp));
			strncpy(temp, p, 4);
			p = p + 4;
			len = atoi(temp);
			strncpy(args[i], p, len);
			p = p + len;
			i++;
		}
		for(int i = 0; i < 9; i++)
		{
			printf("args[%d] = %s\n",i,args[i]);
		}
		/* 修改ret 插入成功时 ret = 1, 否则ret = 0 */
		bool result = insertUsers("users", "9", args[0],args[1],args[2],
			args[3],args[4],args[5],args[6],args[7],args[8]);
		if(!result)		//插入注册用户信息失败，即注册失败
			ret = 1;
		else			//注册成功，时创建新用户的聊天信息表：userName_chatInfo
		{
			char tableName[50] = {0};
			sprintf(tableName, "%s_chatInfo", args[0]);
			createChatInfoTable(tableName);		//注册成功时创建该用户的聊天信息表
		}
	}
	memset(temp, 0, sizeof(temp));
	strcpy(temp, (ret == 0?"yes":"no"));
	memset(*msg, 0, sizeof(*msg));
	sprintf(*msg, "%4d", strlen(temp));
	strcat(*msg, temp);
}

void processClientExit(int *send_fd,map<string,int> * userSocketMap)
{
	string userName = "";
	pthread_mutex_lock(pMutex);
	map<string,int>::iterator iter = userSocketMap->begin();
	for(;iter != userSocketMap->end();iter++)
	{
		if(iter->second == *send_fd)
		{
			userName = iter->first;
			userSocketMap->erase(iter);
			break;
		}
	}
	pthread_mutex_unlock(pMutex);
	printf("exit user name = [%s]\n",userName.c_str());
	if(userName != "")
		saveExitLog(userName);
}

/* 登陆请求函数 *msg : [长度][数据段][长度][数据段]。。。。。。 */
void processLogin(char ** msg, int *send_fd, map<string,int> * userSocketMap)
{
	char username[20] = {0};
	char password[10] = {0};
	char temp[20] = {0};
	char *p = *msg;
	strncpy(temp, p, 4);
	p = p + 4;
	strncpy(username, p, atoi(temp));
	p = p + atoi(temp);
	/*检查用户是否已经在线 以下两行需要加互斥锁*/
	pthread_mutex_lock(pMutex);
	map<string,int>::iterator iter = userSocketMap->find(string(username));
	bool online = (iter != userSocketMap->end()?true:false);
	pthread_mutex_unlock(pMutex);
	if(online)/*查找到元素时说明用户已经在线*/
	{
		memset(*msg, 0, sizeof(*msg));
		strcpy(*msg,"   6online");
		return;
	}
	strncpy(temp, p, 4);
	p = p + 4;
	strncpy(password, p, atoi(temp));
	int ret = 0;
	if(!isInited())
	{
		initMysql();
	}
	/* 检查用户名和密码，正确返回0，不存在返回1，密码不正确返回2*/
	ret = checkUser(username,password);
	//yes null no
	memset(*msg, 0, sizeof(*msg));
	memset(temp, 0, sizeof(temp));
	if(ret == 0)			/*登陆成功*/
	{
		strcpy(temp,"yes");
		userSocketMap->insert(pair<string, int>(string(username),*send_fd));
		/*id username time ip port*/
		struct sockaddr_in peeraddr;
		socklen_t len = sizeof(peeraddr);
		getpeername(*send_fd, (sockaddr*)&peeraddr, &len);
		char *pname = username;
		/*保存登陆日志*/
		saveLoginLog(pname, inet_ntoa(peeraddr.sin_addr), ntohs(peeraddr.sin_port));
		/*导出登陆用户需要同步的表的未同步的记录*/
		exportTable(pname, inet_ntoa(peeraddr.sin_addr));
		/*发送导出的同步信息文件*/
		sendSyncFile(pname, *send_fd);
	}
	else if(ret == 1)		/*用户不存在*/
		strcpy(temp, "null");
	else 				/*密码错误*/
		strcpy(temp, "no");
	sprintf(*msg, "%4d", strlen(temp));
	strcat(*msg, temp);
}

//消息转发处理函数
//*msg格式：[类型][接收用户名长][接收用户名][消息长度][消息]
void processRelay(char ** msg,int * send_fd, map<string,int> * userSocketMap)
{
	char *p = *msg;
	char temp[2048] = {0};
	char recvName[30] = {0};
	strncpy(temp,p,4);
	int flag = atoi(temp);			//类型3:文字 7:语音
	p += 4;
	strncpy(temp,p,4);			//取接收用户名长度
	p += 4;
	strncpy(recvName,p,atoi(temp));		//取接收用户名
	p += atoi(temp);
	//如果接收方不在线，则将消息存入数据路，当用户登陆时发送
	strcpy(temp,p);
	pthread_mutex_lock(pMutex);
	map<string,int>::iterator iter = userSocketMap->begin();
	for(;iter != userSocketMap->end(); iter++)	//找到发送方的用户名
	{
		if(iter->second == *send_fd)
			break;
	}
	int recv_fd = (*userSocketMap)[string(recvName)];	//接收端套接字
	pthread_mutex_unlock(pMutex);
	string sendName = iter->first;				//发送发用户名
	char * pMsg = NULL;
	char * url  = NULL;
	if(flag == 3)
	{
		pMsg = temp + 4;				//获取消息（不含长度）
	}
	else if(flag == 7 || flag == 8 || flag == 9)		//语音、图片、文件都存储url
	{
		url = temp + 4;
	}
	saveChatInfo(sendName, string(recvName), flag, pMsg, url);
	if(recv_fd == 0)	//原带有flag的消息加上消息长度头部后存入数据库
	{
		return;
	}
	memset(*msg + 4, 0, sizeof(*msg));		//保留 flag 部分
	sprintf(*msg + 4,"%4d",strlen(sendName.c_str()));	//在flag后面追加发送用户名长度
	strcat(*msg,sendName.c_str());		     	//追加发送用户名
	strcat(*msg,temp);				//追加消息长度和消息
	//将char * 的前面加上四个字节的长度
	addPacketLen(msg);
	pthread_mutex_lock(pMutex);
        int ret = writen(recv_fd, *msg, strlen(*msg));
	pthread_mutex_unlock(pMutex);
	printf("sendsize = %d,msg = [%s], recv_fd = %d\n",ret,*msg,recv_fd);
}

//消息处理函数
//*msg数据格式：[类型][长度][数据段][长度][数据段]。。。。。。
void * process(struct MsgProcessPacket * args)/*char ** msg*/
{
	struct MsgProcessPacket *pPacket = args;
	pMutex = args->pMutex;
	char *msg = args->msg;
	int send_fd = pPacket->send_fd;
	int flag = 0;
	char temp[1024] = {0};
	strncpy(temp, msg, 4);
	flag = atoi(temp);
	char * pdata = msg + 4;	//越过类型，指向第一个数据
	if(flag == 0)		//处理客户端退出
		processClientExit(&send_fd,pPacket->userSocketMap);
	else if(flag == 1)	//来自客户端的注册请求
		processRegister(&pdata);
	else if(flag == 2)	//来自客户端的登陆请求
		processLogin(&pdata,&send_fd,pPacket->userSocketMap);			
	else if(flag == 3 || flag == 7 || flag == 8 || flag == 9)//转发消息
	{
		/*3:文字消息 7：语音消息 8:图片消息 9：文件消息*/
		char * p = msg;
		processRelay(&p, &send_fd, pPacket->userSocketMap);
		return NULL;
	}
	else if(flag == 4)	//接收客户端文件
	{
		processRecvFile(&pdata, send_fd);
		return NULL;
	}
	else if(flag == 5)	//向客户端发送文件
	{
		processSendFile(&pdata, send_fd);
		return NULL;
	}
	else if(flag == 6)	//处理客户端的修改信息请求
	{
		processModifyInfo(&pdata);
		return NULL;
	}
	else
		return NULL;
	sprintf(temp, "%4d", strlen(msg));
	strcat(temp, msg);
	strcpy(msg, temp);
        int ret = writen(send_fd, msg, strlen(msg)); 
        outInfo(msg + 4);
        printf("send size : %d,send msg : %s\n",ret,msg);
	return NULL;
}

void* sendFile(void* args)
{
	FILE *file = fopen("./processMsg.h", "r");
	if(file == NULL)
	{
		printf("file open fail\n");
		return NULL;
	}
	char buf[1024] = {0};
	char* p = buf;
	while(fread(p,sizeof(char),sizeof(p) - 1,file))
	{
		printf("%s",p);
	}
	printf("\n");
	return NULL;
}

