#include "processMsg.h"

#define READ_NUM 1025
#define MAX_SEND_SIZE 1224 /*固定长度 28 + 文件名(最长) + 数据段(最长1024) + 1位空字符*/

#define BUF_SIZE 4096
#define MAX_SIZE 4296

extern pthread_mutex_t *pMutex;
int times = 0;

void sendFile(const char *filePath, const int fd)
{
	FILE * file;
	int maxSection;
	char buf[READ_NUM] = {0};
	char sendbuf[MAX_SEND_SIZE] = {0};
	printf("存在同步文件[%s]\n", filePath);
	if((file = fopen(filePath, "r")) == NULL)
		return;
	long fileSize = 0;
	fseek(file, 0L, SEEK_END);
	fileSize = ftell(file);
	fseek(file, 0L, SEEK_SET);

	if(fileSize != 0)		//打开文件非空
	{
		printf("file %s is not empty! size = [%ld]\n",filePath,fileSize);
		string fileName = string(filePath);
		fileName = fileName.substr(fileName.find_last_of('/') + 1);
		fseek(file, 0, SEEK_END);
		maxSection = (int)(ftell(file) / READ_NUM) + 1;	//获取分段的总数
		fseek(file, 0, SEEK_SET);
		int i = 1;
		while(fread(buf, sizeof(char), READ_NUM - 1, file) > 0)//有数据可读
		{
			/*发送文件的格式 总长度+类型+文件名长+文件名+分段编号+最大编号+数据段长度+数据段
 			*  		 [  4 ]+[ 4]+[  4   ]+[ *  ]+[  8位 ]+[  8位 ]+[   4位  ]+[ *  ]*/
			sprintf(sendbuf,"%4d%4d%4d%s%8d%8d%4d%s", 28+strlen(fileName.c_str())+strlen(buf),20,strlen(fileName.c_str()),fileName.c_str(),i++,maxSection,strlen(buf),buf);
			pthread_mutex_lock(pMutex);
			int ret = writen(fd, sendbuf, strlen(sendbuf));
			printf("sendsize = %d endbuf = [%s]\n", ret, sendbuf);

			pthread_mutex_unlock(pMutex);
			memset(buf,0,sizeof(buf));
			memset(sendbuf,0,sizeof(sendbuf));
		}
		printf("\n");
	}
	else
		printf("file \'%s\' is empty\n", filePath);
	fclose(file);
}

/* 发送哪个用户的同步文件*/
void sendSyncFile(const char *userName, const int fd)
{
	char fileName[100] = {0};
	//sprintf(fileName, "/tmp/%s_chatInfo.txt", userName);	//聊天信息
	//if(!access(fileName, F_OK))
	//{
	//	sendFile(fileName, fd);
	//	remove(fileName);
	//}
	memset(fileName, 0, sizeof(fileName));
	sprintf(fileName, "/tmp/%s_recent_chatList.txt", userName);	 	//最近聊天列表
	if(!access(fileName, F_OK))
	{
		sendFile(fileName, fd);
	//	remove(fileName);
	}
	memset(fileName, 0, sizeof(fileName));
	sprintf(fileName, "/tmp/%s_user_friendList.txt", userName, userName);	//用户好友列表
        if(!access(fileName, F_OK))
	{
		sendFile(fileName, fd);
	//	remove(fileName);
	}
        memset(fileName, 0, sizeof(fileName));
	sprintf(fileName, "/tmp/user_info.txt");		//好友账户信息
        if(!access(fileName, F_OK))
	{
                sendFile(fileName, fd);
	//	remove(fileName);
	}
        memset(fileName, 0, sizeof(fileName));
	sprintf(fileName, "/tmp/users.txt");		//好友基本信息
        if(!access(fileName, F_OK))
	{
                sendFile(fileName, fd);
	//	remove(fileName);
	}
}

void processRecvFile(char ** msg, int client_fd)/*接收客户端client_fd套接字发送的文件*/
{
/*msg格式 文件名长+文件名+文件大小
          [  4   ]+[ *  ]+[  8位 ]*/
	int len;
	char fileName[100] = {0}, filePath[100] = {0}, temp[20] = {0};
	char * p = *msg;
	//long  currentNum;
	//long  maxNum;
	long fileSize;
	strncpy(temp, p, 4); p += 4; len = atoi(temp);
	strncpy(fileName, p, len); p += len;	//文件名
	strncpy(temp, p, 8); p += 8;
	fileSize = atol(temp);

	char * pFileName = fileName;
	strcpy(filePath, string("/tmp/").c_str());
	strcat(filePath, pFileName);
	if(access(filePath, F_OK) != -1)
		remove(filePath); /*先删除原同名文件*/

	FILE *writeFile = fopen(filePath, "a");/*打开接收文件*/
	if(writeFile == NULL)
		printf("open writeFile fail\n");
	else
		printf("open writeFile success\n");

	/*循环接收数据并写入数据*/
	long recvSize = 0;
	char recvbuf[MAX_SIZE] = {0};
	while(1){
		int recvLen = recv(client_fd, recvbuf, MAX_SIZE, 0);
		len = fwrite(recvbuf, sizeof(char), recvLen, writeFile);
		recvSize += len;
		times++;
	printf("filePath(%s),recvSize(%d),writeSize(%d),times(%d)\n",filePath,recvSize,len,times);
		if(recvSize == fileSize){
			times = 0;
			break;
		}
		memset(recvbuf, 0, sizeof(recvbuf));
	}
	fclose(writeFile);
}

void processSendFile(char **msg, int client_fd)/*向客户端client_fd套接字发送文件*/
{
/*msg格式 文件名长+文件名	文件信息格式：文件名长+文件名+文件长度
          [  4   ]+[ *  ]		      [   4  ]+[  * ]+[  8  ]*/
	char * p = *msg;
	char temp[100] = {0};
	char fileName[100] = {0};
	char filePath[100] = {0};
	char sendbuf[MAX_SIZE] = {0};
	long fileSize = 0;
	strncpy(temp, p, 4);	//读取文件长度
	p += 4;
	strncpy(fileName, p, atoi(temp));//读取文件名
	sprintf(filePath, "/tmp/%s", fileName);//设置文件路径
	FILE * sendFile = fopen(filePath, "r");
		
	if(sendFile == NULL){//文件不存在
		printf("open file [%s] fail!\n", filePath); 
		strcpy(sendbuf,"__NOT_EXIST__");
		int len = strlen("__NOT_EXIST__");
		int writeLen = write(client_fd, sendbuf, len);
		//printf("file[%s],readlen[%d],writeSize[%d]\n", fileName, len, writeLen);
		return;
	}
	else{
		fseek(sendFile, 0L, SEEK_END);
		fileSize = ftell(sendFile);
		fseek(sendFile, 0L, SEEK_SET);
	}
	if(fileSize == 0)//文件为空
	{
		printf("file [%s] size is 0\n", filePath); 
		strcpy(sendbuf,"__EMPTY__");
		int len = strlen("__EMPTY__");
		int writeLen = write(client_fd, sendbuf, len);
		//printf("file[%s],readlen[%d],writeSize[%d]\n", fileName, len, writeLen);
		return;
	}
	sprintf(sendbuf, "%4d%s%8ld", strlen(fileName), fileName, fileSize);//发送文件信息的缓冲区
	printf("send file [%s]\n", sendbuf);
	write(client_fd, sendbuf, strlen(sendbuf));//发送文件信息
	memset(sendbuf, 0, sizeof(sendbuf));

	usleep(20000);//暂停20毫秒
	//sleep(1); //暂停一秒再发送数据
	
	long sendSize = 0;
	while(!feof(sendFile))
	{
		int len = fread(sendbuf, sizeof(char), BUF_SIZE, sendFile);
		int writeLen = write(client_fd, sendbuf, len);
		sendSize += writeLen;
		//printf("file[%s],readlen[%d],writeSize[%d],sendSize[%ld]\n", fileName, len, writeLen, sendSize);
		memset(sendbuf, 0, sizeof(sendbuf));
		usleep(100000);//间隔100毫秒发送数据
		//sleep(1);
	}
}

void processModifyInfo(char ** msg)
{
	//enum ModifyFlag{imageUrl = 0,userName,number,address,personalSignature,phone};
	/*const char * modifyFlags[6] = {string("imageUrl").c_str(),\
				 string("userName").c_str(),\
				 string("number").c_str(),\
				 string("address").c_str(),\
				 string("personalSignature").c_str(),\
				 string("phone").c_str()};*/
	const char * modifyFlags[6] = {"imageUrl","userName","number","address","personalSignature","phone"};
	/*服务器接收消息格式：总长+类型+修改类型+用户名长度+用户名+信息长度+信息*/
	/*msg格式：修改类型+用户名长度+用户名+信息长度+信息*/
	printf("修改信息recv msg: [%s]\n", *msg);
	//1.获取修改类型 2.获取修改用户名 3.获取修改信息
	char * p = *msg;
	int modifyFlag = -1;//0:imageUrl 1:userName 2:number 3:address 4:personalSignature 5:phone
	int len = 0;
	char temp[1024] = {0};
	char userName[100] = {0};
	strncpy(temp, p, 4); p += 4;	//修改类型
	modifyFlag = atoi(temp);
	strncpy(temp, p, 4); p += 4;	//用户名长度
	len = atoi(temp);
	strncpy(userName, p, len); p += len;//用户名
	strncpy(temp, p, 4); p += 4;	//信息长度 此时p指向信息首地址
	len = atoi(temp);		
	char tableName[20] = "user_info";
	if(modifyFlag == 0)		//修改头像需要加上文件路径前缀/tmp/
	{
		sprintf(temp,"/tmp/%s", p);
		p = temp;
	}
	if(modifyFlag == 5)		//如果修改电话，则更新users表
		strcpy(tableName, "users");
	updateTableField(tableName,"username",userName,modifyFlags[modifyFlag],p);	
	if(strcmp(modifyFlags[modifyFlag], "username"))//用户名需要将users表中的信息也修改（还有其他表待完善）
		updateTableField("users","username",userName,modifyFlags[modifyFlag],p);	
}

void processDeleteFriend(char ** msg, int *send_fd, map<string,int> * userSocketMap)
{
	char userName[50] = {0};
	char userName2[50] = {0};
	char temp[1024] = {0};
	char *p = *msg;
	strncpy(temp,p,4); p += 4;
	strncpy(userName,p,atoi(temp)); p += atoi(temp);
	strncpy(temp,p,4); p += 4;
	strncpy(userName2,p,atoi(temp));
	printf("userName = [%s],userName2 = [%s]\n", userName,userName2);
	deleteFriend(userName,userName2);	//删除好友
	//查找userName2用户的登陆套接字
	pthread_mutex_lock(pMutex);
	map<string,int>::iterator iter = userSocketMap->find(userName2);
	if(iter == userSocketMap->end())		//userName2不在线
	{
		pthread_mutex_unlock(pMutex);
		return;
	}
	int recv_fd = iter->second;		//向userName2发送被删除的消息 msg = 名2长+名2+名1长+名1
	pthread_mutex_unlock(pMutex);
	memset(*msg,0,strlen(*msg));
	sprintf(*msg,"%4d%4d%s%4d%s",10,strlen(userName2),userName2,strlen(userName),userName);
	//将char * 的前面加上四个字节的长度
	addPacketLen(msg);
	pthread_mutex_lock(pMutex);
	int ret = writen(recv_fd, *msg, strlen(*msg));
	pthread_mutex_unlock(pMutex);
	printf("sendsize = %d,msg = [%s], recv_fd = %d\n",ret,*msg,recv_fd);
} 

void processAddFriend(char **msg, int *send_fd, map<string,int> *userSocketMap)
{
	char userName[50] = {0};
	char userName2[50] = {0};
	char temp[1024] = {0};
	char *p = *msg;
	strncpy(temp,p,4); p += 4;
	strncpy(userName,p,atoi(temp)); p += atoi(temp);
	strncpy(temp,p,4); p += 4;
	strncpy(userName2,p,atoi(temp));
	printf("userName = [%s],userName2 = [%s]\n", userName,userName2);
	//userName1请求添加userName2
	pthread_mutex_lock(pMutex);
	map<string,int>::iterator iter = userSocketMap->find(userName2);
	if(iter == userSocketMap->end())		//userName2不在线
	{
		pthread_mutex_unlock(pMutex);
		return;
	}
	int recv_fd = iter->second;//向userName2发送添加请求的消息 msg = 名1长+名1+名2长+名2
	pthread_mutex_unlock(pMutex);
	//导出user_info和users表信息
	exportOneTable("user_info");
	exportOneTable("users");
	//发送user_info.txt和users.txt文件
	sendFile("/tmp/user_info.txt",recv_fd);
	sendFile("/tmp/users.txt",recv_fd);
	//temp存储转发消息 类型11
	sprintf(temp,"%4d%4d%s", 4+strlen(*msg), 11, *msg);
	pthread_mutex_lock(pMutex);
	int ret = writen(recv_fd, temp, strlen(temp));
	pthread_mutex_unlock(pMutex);
	printf("sendsize = %d,msg = [%s], recv_fd = %d\n",ret,temp,recv_fd);
}

void processAddFriendResponse(char **msg, int *send_fd, map<string,int> *userSocketMap)
{
	char userName[50] = {0};
        char userName2[50] = {0};
        char temp[1024] = {0};
        char *p = *msg;
        strncpy(temp,p,4); p += 4;
        strncpy(userName,p,atoi(temp)); p += atoi(temp);
        strncpy(temp,p,4); p += 4;
        strncpy(userName2,p,atoi(temp));
        printf("userName = [%s],userName2 = [%s]\n", userName,userName2);
	//存入数据路user_friendList表中
	insertFriendList(userName, userName2);
        //userName请求添加userName2(此消息由userName2响应，发送到userName)
        pthread_mutex_lock(pMutex);
        map<string,int>::iterator iter = userSocketMap->find(userName);
        if(iter == userSocketMap->end())                //userName不在线
        {       
        	pthread_mutex_unlock(pMutex);
                return;		//userName不在线直接退出
        }
        int recv_fd = iter->second;//向userName2发送添加请求的消息 msg = 名1长+名1+名2长+名2
        pthread_mutex_unlock(pMutex);
        //temp存储转发消息 类型11
        sprintf(temp,"%4d%4d%s", 4+strlen(*msg), 12, *msg);
        pthread_mutex_lock(pMutex);
        int ret = writen(recv_fd, temp, strlen(temp));
        pthread_mutex_unlock(pMutex);
        printf("sendsize = %d,msg = [%s], recv_fd = %d\n",ret,temp,recv_fd);
}
