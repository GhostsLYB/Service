#ifndef _MYSQLHELP
#define _MYSQLHELP

#include <mysql/mysql.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string>
#include <iostream>
#include <vector>
#include <iomanip>
using namespace std;


/* 创建连接 返回MYSQL*   */
MYSQL * createconn();

/* 将全局变量mysql初始化*/
void initMysql();

bool isInited();

/* 查找users表中是否存在username的记录
 * 返回查询记录的行数数，不存时为0 */
int selectUser(char* username);

/* 向数据库中添加纪录, 返回受影响（insert）的行*/
bool insertUsers(const char *tableName,const char *arg, ... );

/* 检查用户名和密码，正确返回0，不存在返回1，密码不正确返回2*/
int checkUser(char *username,char *password);

/*判断表是否存在 mysql (已连接的test数据库中)*/
bool isExistTable(const char * tableName);

//userName表示该表属于哪个用户；peerName表示和哪个用户的聊天；chatInfo固定格式表示聊天信息
void createChatInfoTable(const char * tableName);

void deleteTable(const char* tableName);

void getCurrentTime(char * timebuf);

void saveLoginLog(const char* userName, const char* ip, const int port);

/*保存发送段和接收端聊天信息*/
void saveChatInfo(string sendName, string recvName,int flag, const char *pMsg , const char *url);

void showTable(const char * tableName);

/* 关闭数据库连接mysql */
void close(MYSQL * mysql);

/***********以下函数定义在mysqlHelp2.cpp中***********/

/*保存退出时的日志信息*/
void saveExitLog(const string userName);

void exportTable(const char *userName, const char *ip);

void updateTableField(const char *tableName, const char *conditionField, const char *conditionValue,
		      const char *modifyField, const char *modifyValue);

void deleteFriend(const char *userName,const char *userName2);

#endif
