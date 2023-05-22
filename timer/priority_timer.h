#ifndef PRIORITY_TIMER
#define PRIORITY_TIMER

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <queue> 
#include <vector>
#include <time.h>
#include <chrono>
#include <unordered_map>

using namespace std;

typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef std::chrono::seconds S;

typedef Clock::time_point TimeStamp;

class util_timer;

struct client_data {
	sockaddr_in address;
	int sockfd;
	util_timer* timer;
};

class util_timer {
public:
	util_timer() {}
public:
    int id;//用来标记定时器

    TimeStamp expires;//设置过期时间

	void(*cb_func)(client_data*); //设置一个回调函数用来方便删除定时器时将对应的HTTP连接关闭

	client_data* user_data;

    bool operator<(const util_timer& t) {
        return expires < t.expires;
    }
};

class sort_timer_queue {
public:
	sort_timer_queue() {};
	~sort_timer_queue();

	void add_timer(int id, util_timer* timer);

	void adjust_timer(int id); 

	void doWork(int id); //删除指定标识的定时任务

	int tick();

    void pop();



private:
	void del_(size_t i); //删除指定下标的定时任务

	void siftup_(size_t i);//向上调整

    bool siftdown_(size_t index, size_t n);//向下调整

    void SwapNode_(size_t i, size_t j);//交换两个结点位置

    vector<util_timer*> heap_; //一般都用数组来表示堆

    unordered_map<int, size_t> ref_; //key: fd, value: vector中的下标

};

class Utils {
public:
	Utils() {}
	~Utils() {}	


    void init(int timeslot);

    //对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);
public:
	static int *u_pipefd;
    sort_timer_queue m_timer_que;
    static int u_epollfd;
    int m_TIMESLOT;
};

void cb_func(client_data *user_data);


#endif