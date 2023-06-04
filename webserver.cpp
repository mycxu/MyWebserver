#include "webserver.h"
using namespace std;
webserver::webserver() {
    //http_conn类对象
    users = new http_conn[MAX_FD];

    //root文件夹路径
    char server_path[200];
    getcwd(server_path, 200); //getcwd()会将当前工作目录的绝对路径复制到参数buffer所指的内存空间中,参数maxlen为buffer的空间大小。
    char root[6] = "/root";
    m_root = (char *)malloc(strlen(server_path) + strlen(root) + 1);
    strcpy(m_root, server_path);
    strcat(m_root, root);

    //定时器
    users_timer = new client_data[MAX_FD];
}

webserver::~webserver() {
    close(m_epollfd);
    close(m_listenfd);
    close(m_pipefd[1]);
    close(m_pipefd[0]);
    delete[] users;
    delete[] users_timer;
    delete m_pool;
}



void webserver::init(int port, int opt_linger, int trigmode, int thread_num, int close_log, int log_write)
{
    m_port = port;
    m_thread_num = thread_num;
    m_OPT_LINGER = opt_linger;
    m_TRIGMode = trigmode;
    m_close_log = close_log;
    m_log_write = log_write;
}

void webserver::trig_mode()
{
    //LT + LT
    if (0 == m_TRIGMode)
    {
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 0;
    }
    //LT + ET
    else if (1 == m_TRIGMode)
    {
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 1;
    }
    //ET + LT
    else if (2 == m_TRIGMode)
    {
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 0;
    }
    //ET + ET
    else if (3 == m_TRIGMode)
    {
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 1;
    }
}

void webserver::log_write() {
    if(m_close_log == 0) {
        if(m_log_write == 1) {
            Log::get_instance()->init("./ServerLog/ServerLog", m_close_log, 2000, 800000, 800);
        }
        else {
            Log::get_instance()->init("./ServerLog/ServerLog", m_close_log, 2000, 800000, 0);
        }
    }
}

void webserver::thread_pool()
{
    //线程池
    m_pool = new threadpool<http_conn>(m_thread_num);
}

void webserver::eventListen()
{
    //网络编程基础步骤
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0); //只有一个,connfd有多个
    assert(m_listenfd >= 0);

    //优雅关闭连接
    if (0 == m_OPT_LINGER)
    {   
        //close()立刻返回，底层会将未发送完的数据发送完成后再释放资源，即优雅退出。
        struct linger tmp = {0, 1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }
    else if (1 == m_OPT_LINGER)
    {
        //close()不会立刻返回，内核会延迟一段时间，这个时间就由l_linger的值来决定。如果超时时间到达之前，发送完未发送的数据(包括FIN包)并得到另一端的确认，close()会返回正确，socket描述符优雅性退出。
        //否则，close()会直接返回错误值，未发送数据丢失，socket描述符被强制性退出。需要注意的时，如果socket描述符被设置为非堵塞型，则close()会直接返回值。
        struct linger tmp = {1, 1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(m_port);

    int flag = 1; //reuse
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    
    ret = bind(m_listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0);
    ret = listen(m_listenfd, 5);
    assert(ret >= 0);

    utils.init(TIMESLOT);

    //epoll创建内核事件表
    epoll_event events[MAX_EVENT_NUMBER];
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);

    utils.addfd(m_epollfd, m_listenfd, false, m_LISTENTrigmode);
    http_conn::m_epollfd = m_epollfd;

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);
    assert(ret != -1);
    utils.setnonblocking(m_pipefd[1]); //send是将信息发送给套接字缓冲区，如果缓冲区满了，则会阻塞，这时候会进一步增加信号处理函数的执行时间，为此，将其修改为非阻塞。
    utils.addfd(m_epollfd, m_pipefd[0], false, 0);

    utils.addsig(SIGPIPE, SIG_IGN);
    utils.addsig(SIGALRM, utils.sig_handler, false);
    utils.addsig(SIGTERM, utils.sig_handler, false);

    alarm(TIMESLOT);

    //工具类,信号和描述符基础操作
    Utils::u_pipefd = m_pipefd;
    Utils::u_epollfd = m_epollfd;
}

void webserver::timer(int connfd, struct sockaddr_in client_address)
{
    users[connfd].init(connfd, client_address, m_root, m_CONNTrigmode);

    //初始化client_data数据
    //创建定时器，设置回调函数和超时时间，绑定用户数据，将定时器添加到小顶堆中
    users_timer[connfd].address = client_address;
    users_timer[connfd].sockfd = connfd;

    util_timer *timer = new util_timer;
    timer->id = connfd;
    timer->user_data = &users_timer[connfd];
    timer->cb_func = cb_func;

    TimeStamp cur = Clock::now();
    timer->expires = cur + MS(15000);

    users_timer[connfd].timer = timer;
    utils.m_timer_que.add_timer(connfd, timer); 
}

void webserver::adjust_timer(int id)
{
    utils.m_timer_que.adjust_timer(id); //函数内部重置超时时间
    LOG_INFO("%s", "adjust timer once");
}

void webserver::deal_timer(util_timer *timer, int sockfd)
{
    timer->cb_func(&users_timer[sockfd]);
    if (timer)
    {
        utils.m_timer_que.doWork(sockfd);
    }
    LOG_INFO("close fd %d", users_timer[sockfd].sockfd);
}

bool webserver::dealClientData()
{
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    if (0 == m_LISTENTrigmode) // LT
    {
        int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
        if (connfd < 0)
        {
            LOG_ERROR("%s:errno is:%d", "accept error", errno);
            return false;
        }
        if (http_conn::m_user_count >= MAX_FD)
        {
            utils.show_error(connfd, "Internal server busy");
            LOG_ERROR("%s", "Internal server busy");
            return false;
        }
        timer(connfd, client_address);
    }

    else // ET
    {
        while (1)
        {   
            //对于accept没有使用循环接收，则会导致当两个连接同时接入的时候，只触发一次，则accept一次，
            //另外一个则停留在SYN队列中。当终端超时后发送FIN状态，这边只是将该连接标识为只读状态。
            //并连接处于CLOSE_WAIT状态。当下一次接入的时候，触发epoll，这次accept的却是上一次的连接，
            //这次的连接依然停留在SYN队列中。如果后续都是单次触发的话，则会导致后续交易都失败。
            int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
            if (connfd < 0)
            {
                LOG_ERROR("%s:errno is:%d", "accept error", errno);
                break;
            }
            if (http_conn::m_user_count >= MAX_FD)
            {
                utils.show_error(connfd, "Internal server busy");
                LOG_ERROR("%s", "Internal server busy");
                break;
            }
            timer(connfd, client_address);
        }
        return false;
    }
    return true;
}

bool webserver::dealWithSignal(bool &timeout, bool &stop_server)
{
    int ret = 0;
    char signals[1024];
    ret = recv(m_pipefd[0], signals, sizeof(signals), 0);
    if (ret == -1)
    {
        return false;
    }
    else if (ret == 0)
    {
        return false;
    }
    else
    {
        for (int i = 0; i < ret; ++i)
        {
            switch (signals[i])
            {
                case SIGALRM:
                {
                    timeout = true;
                    break;
                }
                case SIGTERM:
                {
                    stop_server = true;
                    break;
                }
            }
        }
    }
    return true;
}

void webserver::dealWithRead(int sockfd)
{
    util_timer *timer = users_timer[sockfd].timer;

    //proactor
    if (users[sockfd].read())
    {            
        LOG_INFO("deal with the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));

        //若监测到读事件，将该事件放入请求队列
        m_pool->append(users + sockfd);


        if (timer)
        {
            adjust_timer(sockfd);
        }
    }
    else
    {
        deal_timer(timer, sockfd);
    }
}

void webserver::dealWithWrite(int sockfd)
{
    util_timer *timer = users_timer[sockfd].timer;

    //proactor
    if (users[sockfd].write())
    {
        LOG_INFO("send data to the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));

        if (timer)
        {
            adjust_timer(sockfd);
        }
    }
    else
    {
        deal_timer(timer, sockfd);
    }
    
}

void webserver::eventLoop()
{
    bool timeout = false;
    bool stop_server = false;

    while (!stop_server)
    {
        int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR)
        {
            LOG_ERROR("%s", "epoll failure");
            break;
        }

        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;

            //处理新到的客户连接
            if (sockfd == m_listenfd)
            {
                bool flag = dealClientData();
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                //服务器端关闭连接，移除对应的定时器
                util_timer *timer = users_timer[sockfd].timer;
                deal_timer(timer, sockfd); //里面调用cb_func移除sockfd
            }
            //处理定时信号
            else if ((sockfd == m_pipefd[0]) && (events[i].events & EPOLLIN))
            {
                bool flag = dealWithSignal(timeout, stop_server);
                if (false == flag)
                    LOG_ERROR("%s", "dealclientdata failure");
            }
            //处理客户连接上接收到的数据
            else if (events[i].events & EPOLLIN)
            {
                dealWithRead(sockfd);
            }
            else if (events[i].events & EPOLLOUT)
            {
                dealWithWrite(sockfd);
            }
        }
        if (timeout)
        {
            utils.timer_handler();

            LOG_INFO("%s", "timer tick");

            timeout = false;
        }
    }
}