#include "mysqlHelp.h"

extern MYSQL * mysql;
extern bool inited;

void saveExitLog(const string userName)
{
        char sql[1024]   = {0};
        char curTime[20] = {0};
        getCurrentTime(curTime);
        sprintf(sql, "update login_log set exittime = '%s' where username = '%s' order by id desc limit 1",curTime, userName.c_str());
        printf("saveExitLog = [%s]\n", sql);
        initMysql();
        printf("inited\n");
        int ret = mysql_query(mysql, sql);
        if(ret == 0)
                printf("save exitLog successed\n");
        else
                printf("save exitLog fail\n");
        memset(sql, 0, sizeof(sql));
        sprintf(sql, "delete from login_dynamicInfo where username = '%s'", userName.c_str());
        mysql_query(mysql, sql);
}

void exportTable(const char *userName, const char *ip)
{
	MYSQL_RES * result;
	MYSQL_ROW row;
	char sql[1024] = {0};
	char lastTime[30] = {0};
	char timeCondition[50] = {0};
	int rows = 0;
	sprintf(sql, "select time from login_log where username = '%s' and ip = '%s' order by id desc limit 1", userName, ip);
	initMysql();
	int ret = mysql_query(mysql, sql);
	if(ret == 0)		/*查询登陆日志表成功*/
	{
		result = mysql_store_result(mysql);
		rows = mysql_num_rows(result);
		if(rows == 1)	/*有ip记录，同步上次登录退出后的记录*/
		{
			row = mysql_fetch_row(result);
			strcpy(lastTime, (char*)row[0]);
			mysql_free_result(result);
		}
		/*否则该ip未登陆过该用户, lastTime为空同步全部信息*/
		if(strlen(lastTime) == 0)
			strcpy(timeCondition,"1");
		else
			sprintf(timeCondition, "time >= '%s'", lastTime);
	//userName_userName_chatInfo  userName_recent_chatlist userName_user_friendList userName_user_info
	//userName_chatInfo	      recent_chatList		user_friendList		user_info
	//login_log
		memset(sql, 0, sizeof(sql));
		sprintf(sql, "select * from %s_chatInfo where %s into outfile '/tmp/%s_%s_chatInfo.txt' fields terminated by ' ' enclosed by '\"'", userName, timeCondition, userName, userName);
		printf("export sql = [%s]\n",sql);
		mysql_query(mysql, sql);
		memset(sql, 0, sizeof(sql));
                sprintf(sql, "select * from recent_chatList where username = '%s' and %s into outfile '/tmp/%s_recent_chatList.txt' fields terminated by ' ' enclosed by '\"'", userName, timeCondition, userName);
                printf("export sql = [%s]\n",sql);
                mysql_query(mysql, sql);
		memset(sql, 0, sizeof(sql));
                sprintf(sql, "select * from user_friendList where (requestUser = '%s' or responseUser = '%s') and %s into outfile '/tmp/%s_user_friendList.txt' fields terminated by ' ' enclosed by '\"'", userName, userName, timeCondition, userName);
                printf("export sql = [%s]\n",sql);
                mysql_query(mysql, sql);
		memset(sql, 0, sizeof(sql));
                sprintf(sql, "select * from user_info where username = '%s' or username in (select username from user_friendList where requestUser = '%s' or responseUser = '%s') into outfile '/tmp/%s_user_info.txt' fields terminated by ' ' enclosed by '\"'", userName, userName, userName, userName);
                printf("export sql = [%s]\n",sql);
                mysql_query(mysql, sql);
	}

}

void updateTableField(const char *tableName,const char *conditionField,const char *conditionValue,
                      const char *modifyField, const char *modifyValue)
{
	initMysql();
	char sql[1024] = {};
	sprintf(sql, "update %s set %s = '%s' where %s = '%s'",
		      tableName, modifyField, modifyValue, conditionField, conditionValue);
	printf("sql = [%s]\n", sql);
	int ret = mysql_query(mysql, sql);
	if(ret == 0)
		printf("update successed!\n");
	else
		printf("update failed!\n");
}
