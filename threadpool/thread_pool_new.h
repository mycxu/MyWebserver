#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <cstdio>
#include <exception>
#include <iostream>
#include <list>
#include <pthread.h>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional> 
#include <queue>
#include "../lock/locker.h"

using namespace std;

template<typename T>
class threadpool {
public:
    threadpool(int thread_num = 8, int requests_num = 10000);
    ~threadpool();

    template<typename F> bool append(F&& request);

private:
    static void* worker(void* arg);

    void run();

private:
    int m_thread_num;

    int m_max_requests;

    struct Pool {
        mutex mtx;
        queue<function<void()>> tasks;
        bool isClosed = false;
        condition_variable cond;
    };

    shared_ptr<Pool> pool_;

};

template<typename T> 
threadpool<T>::threadpool(int thread_num, int max_requests): m_thread_num(thread_num), m_max_requests(max_requests), pool_(make_shared<Pool>()){
    if((thread_num <= 0) || (max_requests) <= 0) {
        throw std::exception();
    }

    for(int i = 0; i < thread_num; ++i) {
        thread(
            [pool = pool_] {
                unique_lock<mutex> locker(pool->mtx);
                while(true) {
                    if(!pool->tasks.empty()) {
                        auto task = move(pool->tasks.front());
                        pool->tasks.pop();
                        locker.unlock();
                        task();
                        locker.lock();
                    }
                    else if(pool->isClosed) break;
                    else {
                        pool->cond.wait(locker);
                    }
                }
            }
        ).detach();
    }  

}

template<typename T>
threadpool<T>::~threadpool() {
    //若pool_非空再进来析构，按逻辑是肯定会进到这个if里面，但是还是加一个if保护一下，以免报错
    //static_cast就是把pool_强转成bool
    if(static_cast<bool>(pool_)) {
        {
            lock_guard<std::mutex> locker(pool_->mtx); 
            pool_->isClosed = true;
        }
        pool_->cond.notify_all();  //通知所有线程关闭
    }
}

template<typename T>
template<typename F>
bool threadpool<T>::append(F&& request) {
    {
        //声明一个临时作用域，超出这个作用域则释放锁，相当于先unlock再post
        lock_guard<std::mutex> locker(pool_->mtx); //获得锁
        if(pool_->tasks.size() > m_max_requests) {
            return false;
        }
        //std::bind将client参数绑定到WebServer::OnWrite_方法，这样返回的functor就变为void()，
        //放入到std::function<void()>可调用对象包装器中。
        pool_->tasks.emplace(forward<F>(request));//完美转发
    }
    pool_->cond.notify_one();
    return true;
}


#endif