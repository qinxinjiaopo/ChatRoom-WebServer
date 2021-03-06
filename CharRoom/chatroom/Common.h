#ifndef  CHATROOM_COMMON_H
#define CHATROOM_COMMON_H

#include <iostream>
#include <list> //epoll上的fd连接在双链表上
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 默认服务器端IP地址
#define SERVER_IP "127.0.0.1"

// 服务器端口号
#define SERVER_PORT 8888

// int epoll_create(int size)中的size
// 为epoll支持的最大句柄数
#define EPOLL_SIZE 5000

// 缓冲区大小65535
#define BUF_SIZE 0xFFFF
    
// 新用户登录后的欢迎信息
#define SERVER_WELCOME "Welcome you join to the chat room! Your chat ID is: Client #%d"

// 其他用户收到消息的前缀
#define SERVER_MESSAGE "ClientID %d say >> %s"

// 退出系统
#define EXIT "EXIT"

// 提醒你是聊天室中唯一的客户
#define CAUTION "There is only one int the char room!"

/**
 addfd()函数的步骤：
 (1)初始化结构体epoll_event，设置用户的fd,设置可读(宏)的fd,设置ET or LT模式
（2）使用epoll_ctl()向epfd注册fd的上的event
 (3)fcntl()设置socket为nonblocking模式
*/

// 注册新的fd到epollfd中
// 参数enable_et表示是否启用ET模式，如果为True则启用，否则使用LT模式
/**
 @function 添加句柄
 @param epollfd(int) 创建的句柄
 @param fd(int) 用户数据的fd文件描述符
 @param enable_et(bool) 是否启用et模式，true则启用，否则是LT模式
 @return void
*/
/** ET和LT的区别
一、ET模式的文件描述符(fd)：

当epoll_wait检测到fd上有事件发生并将此事件通知应用程序后，应用程序必须立即处理该事件，
因为后续的epoll_wait调用将不再向应用程序通知这一事件。
epoll_wait只有在客户端第一次发数据是才会返回,以后即使缓冲区里还有数据，也不会返回了。
epoll_wait是否返回，是看客户端是否发数据，客户端发数据了就会返回，且只返回一次。
eg：客户端发送数据，I/O函数只会提醒一次服务端fd上有数据，以后将不会再提醒
所以要求服务端必须一次把数据读完--->循环读数据 (读完数据后，可能会阻塞)  --->将描述符设置
成非阻塞模式

二、LT模式的文件描述符(fd)：
当epoll_wait检测到fd上有事件发生并将此事件通知应用程序后，应用程序可以不立即处理该事件，
这样，当应用程序下一次调用epoll_wait时，epoll_wait还会再次向应用程序通知此事件，直到此事
件被处理。
eg：客户端发送数据，I/O函数会提醒描述符fd有数据---->recv读数据，若一次没有读完，I/O函数
会一直提醒服务端fd上有数据，直到recv缓冲区里的数据读完
三 结论：ET模式在很大程度上降低了同一个epoll事件被重复触发的次数
*/

static void addfd( int epollfd, int fd, bool enable_et )
{
	/**
	struct epoll_event {
		uint32_t     events;      // Epoll events 
		epoll_data_t data;        // User data variable 
	};
	a.events成员描述事件类型
	EPOLLIN :表示对应的文件描述符可以读(包括对端SOCKET正常关闭)
	EPOLLET: 将EPOLL设为边缘触发(Edge Triggered)模式,这是相对于水平触发
	(Level Triggered)来说的
	b.data用于存储用户数据，是epoll_data_t结构类型

	typedef union epoll_data {
	void        *ptr;
	int          fd;
	uint32_t     u32;
	uint64_t     u64;
	} epoll_data_t;
	*/

    struct epoll_event ev;
    ev.data.fd = fd;
 
	//设置可读的文件描述符
    ev.events = EPOLLIN;//可读
    if( enable_et ) //这里做判断是否启用et模式，true则启用，否则是LT模式
        ev.events = EPOLLIN | EPOLLET;

	/**
	int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
	epfd是创建的句柄，
	op包含三种：
    a.EPOLL_CTL_ADD，向epfd注册fd的上的event
	b.EPOLL_CTL_MOD，修改fd已注册的event
	c.EPOLL_CTL_DEL，从epfd上删除fd的event
	  fd是文件描述符，event指定内核要监听事件,它是struct epoll_event
	结构类型的指针。
	*/
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    // 设置socket为nonblocking模式
    // 执行完就转向下一条指令，不管函数有没有返回。

	/**


fcntl（文件描述词操作）
相关函数  open，flock
 可以用fcntl 函数改变一个已打开的文件的属性,可以重新设置读、写、追加、非阻塞等标志
(这些标志称为File StatusFlag),而不必重新open 文件。

表头文件  #include<unistd.h>
#include<fcntl.h>

定义函数  int fcntl(int fd , int cmd);
int fcntl(int fd,int cmd,long arg);
int fcntl(int fd,int cmd,struct flock * lock);

函数说明  fcntl()用来操作文件描述词的一些特性。参数fd代表欲设置的文件描述词，参数cmd代表欲操作的指令。
有以下几种情况:
F_DUPFD用来查找大于或等于参数arg的最小且仍未使用的文件描述词，并且复制参数fd的文件描述词。
 执行成功则返回新复制的文件描述词。请参考dup2()。F_GETFD取得close-on-exec旗标。若此旗标的FD_CLOEXEC位为0，代表在调用exec()相关函数时文件将不会关闭。
F_SETFD 设置close-on-exec 旗标。该旗标以参数arg 的FD_CLOEXEC位决定。
F_GETFL 取得文件描述词状态旗标，此旗标为open（）的参数flags。
F_SETFL 设置文件描述词状态旗标，参数arg为新旗标，但只允许O_APPEND、O_NONBLOCK和O_ASYNC位的改变，其他位的改变将不受影响。
F_GETLK 取得文件锁定的状态。
F_SETLK 设置文件锁定的状态。此时flcok 结构的l_type 值必须是F_RDLCK、F_WRLCK或F_UNLCK。如果无法建立锁定，则返回-1，错误代码为EACCES 或EAGAIN。
F_SETLKW F_SETLK 作用相同，但是无法建立锁定时，此调用会一直等到锁定动作成功为止。若在等待锁定的过程中被信号中断时，会立即返回-1，错误代码为EINTR。参数lock指针为flock 结构指针，定义如下
struct flcok
{
    short int l_type; // 锁定的状态
	short int l_whence;//决定l_start位置
	off_t l_start; //锁定区域的开头位置
	off_t l_len; //锁定区域的大小
	pid_t l_pid; //锁定动作的进程
};
l_type 有三种状态 :
   F_RDLCK 建立一个供读取用的锁定
   F_WRLCK 建立一个供写入用的锁定
   F_UNLCK 删除之前建立的锁定
l_whence 也有三种方式 :
   SEEK_SET 以文件开头为锁定的起始位置。
   SEEK_CUR 以目前文件读写位置为锁定的起始位置
   SEEK_END 以文件结尾为锁定的起始位置。
返回值  成功则返回0，若有错误则返回 - 1，错误原因存于errno.
*/

	//F_SETFL 设置文件描述词状态旗标，参数arg为新旗标，
	//但只允许O_APPEND、O_NONBLOCK和O_ASYNC位的改变，其他位的改变
	//将不受影响。

	//F_GETFL 取得文件描述词状态旗标，此旗标为open（）的参数flags。

	//设置文件描述符的状态旗标,非阻塞
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0)| O_NONBLOCK);
    printf("fd added to epoll!\n\n");
}

#endif // CHATROOM_COMMON_H