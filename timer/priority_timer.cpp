#include "priority_timer.h"
#include "../http/http_conn.h"


sort_timer_queue::~sort_timer_queue() {
    for(int i = 0; i < heap_.size(); ++i) {
        delete heap_[i];
    }
}

void sort_timer_queue::siftup_(size_t i) {
    assert(i >= 0 && i < heap_.size());
    size_t j = (i - 1) / 2; // i节点的父节点下标为（i - 1）/ 2,
    // i节点的左右节点下标分别为 2 * i + 1 和 2 * i + 2
    // 最小堆非叶子节点的值不大于左右节点的值
    while(j >= 0) {
        if(heap_[j] < heap_[i]) { break; } // 如果父节点已经小于当前节点，就不需要再上升了
        //此时当前节点小于父节点，需要进行调整
        SwapNode_(i, j);
        i = j; // 此时i节点已经被交换至父节点（j）的位置，但i和j的下标值还没有交换，所以需要重新赋值
        j = (i - 1) / 2; //新的父节点下标
    }
}

// 交换俩个节点的位置
void sort_timer_queue::SwapNode_(size_t i, size_t j) { 
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i]->id] = i;
    ref_[heap_[j]->id] = j;
} 

bool sort_timer_queue::siftdown_(size_t index, size_t n) {
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n <= heap_.size());
    size_t i = index;
    size_t j = i * 2 + 1; // i的左节点
    while(j < n) {
        // 如果右节点小于左节点，后面就和右节点交换，否则就和左节点交换。和小的那个子节点交换位置
        // j + 1 < n ，此处的n在del操作中为heap.size() - 1, 而在add操作中为heap.size()
        // 所以删除操作时，变换后的节点在进行下沉操作时，不会与最后一个节点（也就是要删除的那个节点）进行交换
        if(j + 1 < n && heap_[j + 1] < heap_[j]) j++; 
        if(heap_[i] < heap_[j]) break; //如果当前节点已经小于最小的子节点，就不需要再下沉了
        SwapNode_(i, j);
        i = j;
        j = i * 2 + 1; //新的左孩子节点的位置
    }
    return i > index; //如果i没有下沉，就返回false
}

void sort_timer_queue::add_timer(int id, util_timer* timer) {
    assert(id >= 0);
    size_t i;
    if(ref_.count(id) == 0) {
        //如果是一个新的定时器
        i = heap_.size();
        ref_[id] = i;
        heap_.push_back(timer);
        siftup_(i); //向一个小顶堆添加元素，称为shift up
    } 
    else {
        //如果定时器已经存在, 则修改他的定时时间，并重新排序小顶堆
        i = ref_[id];
        heap_[i]->expires = Clock::now() + MS(15000);
        heap_[i]->cb_func = cb_func;
        //下沉操作
        if(!siftdown_(i, heap_.size())) {
            //如果不能下沉，则上升
            siftup_(i);
        }
    }
}

void sort_timer_queue::doWork(int id) {
    
    if(heap_.empty() || ref_.count(id) == 0) {
        return;
    }
    size_t i = ref_[id];
    util_timer* node = heap_[i];
    del_(i);
}

void sort_timer_queue::del_(size_t index) {
    
    assert(!heap_.empty() && index >= 0 && index < heap_.size());
    
    size_t i = index;
    size_t n = heap_.size() - 1;
    assert(i <= n);
    if(i < n) {
        //如果i==n，就直接删除最后一个节点
        SwapNode_(i, n); //把最后一个节点和当前要删除的节点交换
        if(!siftdown_(i, n)) { //然后调整交换后的点的位置，注意此处的n为heap_.size() - 1; 和add操作中不一样
        // n设置为heap_.size() - 1 保证了要删除的那个节点（此时位于数组最后的位置）不会被交换回去
            siftup_(i);
        }
    }
    
    ref_.erase(heap_.back()->id);
    heap_.pop_back();
}


void sort_timer_queue::adjust_timer(int id) {
    
    assert(!heap_.empty() && ref_.count(id) > 0);
    heap_[ref_[id]]->expires = Clock::now() + MS(15000);

    //调整小顶堆，此时增加当前节点的值，所以只能是下沉操作
    siftdown_(ref_[id], heap_.size()); 
}

int sort_timer_queue::tick() {
    printf("trick\n");
    if(heap_.empty()) {
        return 5;
    }
    while(!heap_.empty()) {
        util_timer* node = heap_.front(); // 小顶堆的顶部，也就是最小的那个值
        if(std::chrono::duration_cast<MS>(node->expires - Clock::now()).count() > 0) { // 超时时间 - 当前系统时间 > 0，说明还没到期
            printf("redundent time: %ld\n", std::chrono::duration_cast<MS>(node->expires - Clock::now()).count());
            return std::chrono::duration_cast<S>(node->expires - Clock::now()).count(); // 14127->9127->4127->关闭连接
        }
        node->cb_func(node->user_data);
        pop();
    }

    util_timer* node = heap_.front();
    return std::chrono::duration_cast<S>(node->expires - Clock::now()).count();
}

void sort_timer_queue::pop() {
    assert(!heap_.empty());
    del_(0);
}



void Utils::init(int timeslot) {
    m_TIMESLOT = timeslot;
}

int Utils::setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}


//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//信号处理函数
void Utils::sig_handler(int sig)
{
    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(u_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

//设置信号函数
void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void Utils::timer_handler()
{   
    int rem_expire_time_s = m_timer_que.tick(); //返回还差多少时间到期
    alarm(rem_expire_time_s + 1); // m_TIMESLOT == 5
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

void cb_func(client_data *user_data)
{
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}