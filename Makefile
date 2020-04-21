# 将所有普通数据转化为字符数组发送
# 数据格式：数据长度（4个字节）+数据部分（数据类型（4个字节）+（数据段长度（四个字节）+ 数据））
# 数据举例：[后面数据的总长度][类型][数据段长度][数据段][数据段长度][数据段]......
# processMsg.h 消息处理函数头文件
# mysqlhelp.h  数据库操作函数头文件
# data.h       结构体定义头文件
service:service.o processMsg.o processMsg2.o mysqlHelp.o mysqlHelp2.o
	g++ -g -o service service.o processMsg.o processMsg2.o mysqlHelp.o mysqlHelp2.o -L/usr/lib64/mysql -lmysqlclient -pthread

service.o:service.cpp processMsg.h data.h
	g++ -g -c service.cpp -L/usr/lib64/mysql -lmysqlclient -pthread

processMsg.o:processMsg.cpp processMsg.h
	g++ -g -c processMsg.cpp 

processMsg2.o:processMsg2.cpp processMsg.h
	g++ -g -c processMsg2.cpp 

mysqlHelp.o:mysqlHelp.cpp mysqlHelp.h
	g++ -g -c mysqlHelp.cpp -L/usr/lib64/mysql -lmysqlclient

mysqlHelp2.o:mysqlHelp2.cpp mysqlHelp.h
	g++ -g -c mysqlHelp2.cpp -L/usr/lib64/mysql -lmysqlclient

clean:
	rm -rf a.out *.o

cleantemp:
	rm -rf .*.*.*

cleansyncfile:
	rm -rf /tmp/*.txt

print:
	echo hello world !!!
