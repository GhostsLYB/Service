#include "mysqlHelp.h"

MYSQL * mysql = NULL;
bool inited = false;

/* 创建连接 返回MYSQL*   */
MYSQL * createconn()
{
	MYSQL * conn;

	conn = mysql_init(NULL);
	
	conn = mysql_real_connect(conn, "localhost", "ghost", "123456", "test", 0, NULL, 0);
	
	return conn;
}

/* 将全局变量mysql初始化*/
void initMysql()
{
	if(mysql == NULL)
	{
		mysql = createconn();
		inited = true;
		char sql[30] = {0};
		strcpy(sql,"set names \'utf8\'");
		mysql_query(mysql, sql);
	}
}

bool isInited()
{
	return inited;
}

/* 查找users表中是否存在username的记录
 * 返回查询记录的行数数，不存时为0 */
int selectUser(char* username)
{
	initMysql();
	int res = 0;
	char sql[1024] = {0};
	strcpy(sql,"select * from users where username = '");
	strcat(sql, username);
	strcat(sql, "'");
	res = mysql_query(mysql, sql);
        MYSQL_RES *result = mysql_store_result(mysql);
        res = mysql_num_rows(result);
	return res;
}

/* 向数据库中添加纪录, 返回受影响（insert）的行*/
bool insertUsers(const char *tableName,const char *arg, ... )
{
	initMysql();
	char sql[1024] = {0};
	strcpy(sql,"insert into ");
	strcat(sql, tableName);
	strcat(sql, " values(null,\'");
	va_list ap;
	va_start(ap,arg);
	const char* userName = va_arg(ap,const char*);
	strcat(sql,userName);
	//strcat(sql,va_arg(ap,const char*));
	strcat(sql,"\',\'");
	strcat(sql,va_arg(ap,const char*));
	strcat(sql,"\',\'");
	strcat(sql,(strcmp(va_arg(ap,const char*),"0") == 0)?"男":"女");
	strcat(sql,"\',");
	strcat(sql,va_arg(ap,const char*));	//age
	strcat(sql,",\'");
	strcat(sql,va_arg(ap,const char*));		//emial
	strcat(sql,"\',\'");
	strcat(sql,va_arg(ap,const char*));		//phone
	strcat(sql,"\',\'");
	strcat(sql,va_arg(ap,const char*));		//idnum
	strcat(sql,"\',\'");
	strcat(sql,va_arg(ap,const char*));		//sectet
	strcat(sql,"\',\'");
	strcat(sql,va_arg(ap,const char*));		//answer
	strcat(sql,"\')");
	va_end(ap);
	printf("sql = [%s]\n",sql);
	if(inited)
	{
		char s[20];
		strcpy(s,"set names \'utf8\'");
		mysql_query(mysql,s);	//每次使用result后要在适当位置释放执行的结果集
		mysql_free_result(mysql_store_result(mysql));
		int ret = mysql_query(mysql,sql);
		mysql_free_result(mysql_store_result(mysql));
		if(ret == 0)	//插入users表成功
		{
			char *ps = s;
			getCurrentTime(ps);	//获取当前时间
			sprintf(sql,"insert into user_info values(null,'%s','null','/tmp/icon.png','null','null','%s')",userName,s);
			printf("insert user_info sql = [%s]\n",sql);
			ret = mysql_query(mysql,sql);
			mysql_free_result(mysql_store_result(mysql));
			if(ret == 0)
				return true;
			else
				return false;
		}
		else
			return false;
	}
	return false;
}

/* 检查用户名和密码，正确返回0，不存在返回1，密码不正确返回2*/
int checkUser(char *username,char *password)
{
	initMysql();
	MYSQL_RES * result;
	int ret = 1;
	char sql[1024] = {0};
	strcpy(sql,"select * from users where username = '");
	strcat(sql, username);
	strcat(sql, "'");
	ret = mysql_query(mysql, sql);
        result = mysql_store_result(mysql);
	ret = mysql_num_rows(result);
	if(ret == 0)
		ret = 1;
	else if(ret == 1)
	{
		MYSQL_ROW row = mysql_fetch_row(result);
		if(strcmp(password, row[2]) == 0)
			ret = 0;
		else
			ret = 2;
		mysql_free_result(result);
	}	
	return ret;
}

/*判断表是否存在 mysql (已连接的test数据库中)*/
bool isExistTable(const char * tableName)
{
	initMysql();
	char sql[1024] = {0};
	strcpy(sql, "select table_name from information_schema.tables where TABLE_SCHEMA='test' and TABLE_NAME='");
	strcat(sql, tableName);
	strcat(sql, "'");
	int ret = mysql_query(mysql,sql);
	if(ret != 0)
	{
		printf("judge table exist fail\n");
		return false;
	}
	MYSQL_RES * result = mysql_store_result(mysql);
 	int count = mysql_num_rows(result);
	mysql_free_result(result);
	return (count > 0 ? true : false);
}

//userName表示该表属于哪个用户；peerName表示和哪个用户的聊天；chatInfo固定格式表示聊天信息
void createChatInfoTable(const char * tableName)
{
	initMysql();
	char sql[1024] = {0};
	strcpy(sql, "CREATE TABLE IF NOT EXISTS `");
	strcat(sql, tableName);
	strcat(sql, "` (`id`  int NOT NULL AUTO_INCREMENT ,						\
		     `peerName`  varchar(50) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL ,	\
		     `flag`  int NOT NULL ,								\
		     `direction`  varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL ,	\
		     `word`  varchar(2048) CHARACTER SET utf8 COLLATE utf8_general_ci NULL ,		\
		     `url`  varchar(100) CHARACTER SET utf8 COLLATE utf8_general_ci NULL ,		\
		     `time`  varchar(100) CHARACTER SET utf8 COLLATE utf8_general_ci NULL ,		\
		     PRIMARY KEY (`id`))");
	int ret = mysql_query(mysql, sql);
	printf("sql = [%s]\n",sql);
	if(ret == 0)
		printf("create %s table successed\n", tableName);
	else
		printf("create %s table fail\n", tableName);
}

void deleteTable(const char* tableName)
{
	char sql[1024] = {0};
	strcpy(sql, "drop table ");
	strcat(sql, tableName);
	initMysql();
	int ret = mysql_query(mysql,sql);
	if(ret != 0)
		printf("delete fail\n");
	else
		printf("delete successed\n");

}

void getCurrentTime(char * timebuf)
{
	memset(timebuf,0,sizeof(timebuf));
	time_t alock;
	time(&alock);
	strftime(timebuf,20,"%Y-%m-%d %H:%M:%S",localtime(&alock));
}

void saveLoginLog(const char* userName, const char* ip, const int port)
{
	char tmpbuf[20];
	char *p = tmpbuf;
	getCurrentTime(p);
	/*printf("current time is [%s]\n",p);*/
	char sql[1024] = {0};
	sprintf(sql, "insert into login_log values(null, '%s', '%s', '%s', %d, '%s')",userName,tmpbuf,ip,port,"");
	initMysql();
	int ret = mysql_query(mysql, sql);
	printf("save Login log sql = [%s]\n",sql);
	if(ret == 0)
		printf("save successed\n");
	else
		printf("save fail\n");
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "insert into login_dynamicInfo values('%s', '%s', '%s', %d)",userName, tmpbuf, ip, port);
	mysql_query(mysql, sql);
}

/*保存发送段和接收端聊天信息*/
void saveChatInfo(string sendName, string recvName,int flag, const char *pMsg , const char *url)
{
	char curTime[20];
	char *pCurTime = curTime;
	getCurrentTime(pCurTime);
	char sendSql[1024] = {0};		//发送方和接收方的sql语句
	char recvSql[1024] = {0};
	char tmp[100] = {0};
	sprintf(sendSql, "insert into %s values(null, '%s', %d, 'send', '%s', '%s','%s')",
		string(sendName + "_chatInfo").c_str(), recvName.c_str(), flag,
		pMsg, url, pCurTime);
	printf("sendSql = [%s]\n",sendSql);
	sprintf(recvSql, "insert into %s values(null, '%s', %d, 'recv', '%s', '%s','%s')",
                string(recvName + "_chatInfo").c_str(), sendName.c_str(), flag,
                pMsg, url, pCurTime);
	printf("recvSql = [%s]\n",recvSql);
	if(0 != mysql_query(mysql, sendSql))//插入聊天记录失败
		printf("insert into send chat msg fail\n");
	else
		updateRecentChatList(recvName.c_str(),sendName.c_str(),(flag == 3)?pMsg:url);
	if(0 != mysql_query(mysql, recvSql))//
		printf("insert into recv chat msg fail\n");
	else
		updateRecentChatList(sendName.c_str(),recvName.c_str(),(flag == 3)?pMsg:url);
	
}

void updateRecentChatList(const char *userName, const char *friendName, const char *lastMsg)
{
	initMysql();
	//在recent_chatList中插入或删除一条指定记录
	char sql[200] = {0};
	sprintf(sql, "select count(*) from recent_chatList where userName = '%s' and friendName = '%s'",userName,friendName);
	printf("update recent chat list sql = [%s]\n",sql);
	mysql_query(mysql,sql);
	MYSQL_RES * result = mysql_store_result(mysql);
 	int count = mysql_num_rows(result);
	mysql_free_result(result);
	char currTime[20] = {0};
	char *pTime = currTime;
	getCurrentTime(pTime);
	if(count > 0)//有记录更新
	{
		sprintf(sql, "update recent_chatList set lastMessage = '%s',time = '%s' where userName = '%s' and friendName = '%s'","message",pTime,userName,friendName);
		printf("update recent chat list sql = [%s]\n",sql);
		if(!mysql_query(mysql,sql))
			printf("update recent chat list fail\n");
	}
	else	//没有记录插入
	{
		sprintf(sql, "insert into recent_chatList values(null,'%s','%s','%s','%s','0')",userName,friendName,"message",pTime);
		printf("insert recent chat list sql = [%s]\n",sql);
		if(!mysql_query(mysql,sql))
			printf("insert recent chat list fail\n");
	}
}

void showTable(const char * tableName)
{
	initMysql();
	char sql[1024] = {0};
	sprintf(sql, "select * from %s", tableName);
	int ret = mysql_query(mysql, sql);

	MYSQL_RES*   result = mysql_store_result(mysql);
	MYSQL_FIELD* field  = mysql_fetch_fields(result);
	int 	     rows   = mysql_num_rows(result);
	int 	     cols   = mysql_num_fields(result);
	vector<vector<string> > data;
	int 	     maxFieldLenArray[20] = {0};
	MYSQL_ROW    row;
	if(rows == 0)
	{
		printf("表\"%s\"中没有数据\n",tableName);
		return;
	}
	for(int i = 0; i < rows; i++)		//获取表中的数据
	{
		row = mysql_fetch_row(result);
		vector<string> tmp;
		for(int j = 0; j < cols; j++)
		{
			tmp.push_back((string)row[j]);	
			if(maxFieldLenArray[j] < strlen((char*)row[j]))
				maxFieldLenArray[j] = strlen((char*)row[j]);
		}
		data.push_back(tmp);
	}
	for(int i = 0; i < cols; i++)		//与表头字段长度比较,取较大值作为输出宽度
	{
		if(maxFieldLenArray[i] < strlen(field[i].name))
			maxFieldLenArray[i] = strlen(field[i].name);
	}
	cout << "+";				//输出表格顶部边框
	for(int i = 0; i < cols; i++)
	{
		cout << setw(maxFieldLenArray[i]+2) << setfill('-')<< "-" << setw(0) << "+";
	}
	cout << setfill(' ') << endl;
	cout << "| ";				//输出表头
	for(int i = 0; i < cols; i++)
	{
		cout << setw(maxFieldLenArray[i]) << field[i].name << setw(0) << " | ";
	}
	cout << endl; 
	cout << "+";				//输出表头和数据之间的分割线
	for(int i = 0; i < cols; i++)
	{
		cout << setw(maxFieldLenArray[i]+2) << setfill('-')<< "-" << setw(0) << "+";
	}
	cout << setfill(' ') << endl;
	//输出表数据
	for(vector<vector<string> >::iterator iter = data.begin(); iter != data.end(); iter++)
	{
		int i = 0;
		cout << "| ";
		for(vector<string>::iterator viter = iter->begin(); viter != iter->end(); viter++)
		{
			cout << setw(maxFieldLenArray[i++]) << *viter << setw(0) << " | ";
		} 
		cout << endl;
	}
	cout << "+";				//输出底部边框
	for(int i = 0; i < cols; i++)
	{
		cout << setw(maxFieldLenArray[i]+2) << setfill('-')<< "-" << setw(0) << "+";
	}
	cout << setfill(' ') << endl;
	mysql_free_result(result);
}
