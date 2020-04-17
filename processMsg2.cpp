#include "processMsg.h"

#define READ_NUM 1025
#define MAX_SEND_SIZE 1224 /*固定长度 28 + 文件名(最长) + 数据段(最长1024) + 1位空字符*/

extern pthread_mutex_t *pMutex;

void showFile(const char *filePath, const int fd)
{
	FILE * file;
	int maxSection;
	char buf[READ_NUM] = {0};
	char sendbuf[MAX_SEND_SIZE] = {0};
	printf("存在同步文件[%s]\n", filePath);
	if((file = fopen(filePath, "r")) == NULL)
		return;
	if(!feof(file))		//打开文件非空
	{
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
	sprintf(fileName, "/tmp/%s_%s_chatInfo.txt", userName, userName);	//聊天信息
	if(!access(fileName, F_OK))
	{
		showFile(fileName, fd);
		remove(fileName);
	}
	memset(fileName, 0, sizeof(fileName));
	sprintf(fileName, "/tmp/%s_recent_chatList.txt", userName);	 	//最近聊天列表
	if(!access(fileName, F_OK))
	{
		showFile(fileName, fd);
		remove(fileName);
	}
	memset(fileName, 0, sizeof(fileName));
	sprintf(fileName, "/tmp/%s_user_friendList.txt", userName, userName);	//用户好友列表
        if(!access(fileName, F_OK))
	{
		showFile(fileName, fd);
		remove(fileName);
	}
        memset(fileName, 0, sizeof(fileName));
	sprintf(fileName, "/tmp/%s_user_info.txt", userName, userName);		//好友用户信息
        if(!access(fileName, F_OK))
	{
                showFile(fileName, fd);
		remove(fileName);
	}
	showFile("/tmp/servicetest.txt", fd);
	//showFile("/tmp/homepage.h", fd);
        memset(fileName, 0, sizeof(fileName));
}
