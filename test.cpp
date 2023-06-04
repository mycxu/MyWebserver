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

using namespace std;

template<typename T>
class A {
public:
    template<typename F> void test(F a);
};

template<typename T>
template<typename F>
void A<T>::test(F a) {
    cout<<a<<endl;
}

int main() {
    cout<<"test"<<endl;
    A<int> a;
    a.test(1);
}