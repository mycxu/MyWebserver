#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <cstdio>
#include <exception>
#include <iostream>
#include <list>
#include <pthread.h>
#include "../lock/locker.h"

template<typename T>
class threadpool {
public:
    threadpool(int thread_num = 8, int requests_num = 10000);
    ~threadpool();

    bool append(T* request);

private:
    static void* worker(void* arg);

    void run();

private:
    int m_thread_num;

    int m_max_requests;

    pthread_t *  m_threads;

    std::list<T*> request_workqueue;

    locker m_queuelocker;

    sem m_queuestat;

    bool m_stop;
};

template<typename T> 
threadpool<T>::threadpool(int thread_num, int max_requests): m_thread_num(thread_num), m_max_requests(max_requests), m_stop(false), m_threads(NULL) {
    if((thread_num <= 0) || (max_requests) <= 0) {
        throw std::exception();
    }

    try{
        m_threads = new pthread_t[m_thread_num];
    }
    catch(std::bad_alloc) {
        throw std::exception();
    }  


    for(int i = 0; i < thread_num; ++i) {
        printf("create the %dth thread\n", i);
        if(pthread_create(m_threads + i, NULL, worker, this) != 0) {
            delete[] m_threads;
            throw std::exception();
        }

        if(pthread_detach(m_threads[i])) {
            delete[] m_threads;
            throw std::exception();
        }
    }  


}

template<typename T>
threadpool<T>::~threadpool() {
    delete[] m_threads;
    m_stop = true;
}

template<typename T>
bool threadpool<T>::append(T* request) {
    m_queuelocker.lock();
    if(request_workqueue.size() > m_max_requests) {
        m_queuelocker.unlock();
        return false;
    }
    request_workqueue.push_back(request);
    m_queuelocker.unlock();

    m_queuestat.post(); //post当信号值>=0时
    //为什么先unlock再post？
    //假设当前队列满了，也就是信号量值为0，此时再post就会阻塞，如果此时锁的释放在post后面，那么该线程就会一直拿着锁不释放
    //而其他线程获取不到锁，就会一直等待，就造成了死锁
    return true;
}

template<typename T>
void* threadpool<T>::worker(void* arg) {
    threadpool* pool = (threadpool*)arg;
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run() {

    while(!m_stop) {
        m_queuestat.wait();

        m_queuelocker.lock();
        if(request_workqueue.empty()) {
            m_queuelocker.unlock();
            continue;
        }


        T* request = request_workqueue.front();
        request_workqueue.pop_front();
        m_queuelocker.unlock();
        
        if(!request) {
            continue;
        }

        request->process();
    }
}

#endif