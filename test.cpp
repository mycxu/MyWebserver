#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>
#include <queue>
#include <unordered_map>
#include <chrono>

using namespace std;

typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef std::chrono::seconds S;


typedef Clock::time_point TimeStamp;

// class HeapTimer {
// public:
//     HeapTimer() { heap_.reserve(64); }

//     ~HeapTimer() {}
    
//     void adjust(int id, int newExpires);

//     void add(int id);//添加一个定时器

//     void doWork(int id);//删除指定标识的定时任务

//     void clear();

//     void pop();

//     vector<int> heap_; //一般都用数组来表示堆

// public:
//     void del_(size_t i);////删除指定下标的定时任务
    
//     void siftup_(size_t i);//向上调整

//     bool siftdown_(size_t index, size_t n);//向下调整

//     void SwapNode_(size_t i, size_t j);//交换两个结点位置


//     unordered_map<int, size_t> ref_;
// };

// void HeapTimer::add(int id) {
//     assert(id >= 0);
//     size_t i;
//     if(ref_.count(id) == 0) {
//         //如果是一个新的定时器
//         i = heap_.size();
//         ref_[id] = i;
//         heap_.push_back(id);
//         siftup_(i); //向一个小顶堆添加元素，称为shift up
//     } 
//     else {
//         //如果定时器已经存在, 则修改他的定时时间，并重新排序小顶堆
//         i = ref_[id];
//         //下沉操作
//         if(!siftdown_(i, heap_.size())) {
//             //如果不能下沉，则上升
//             siftup_(i);
//         }
//     }
// }

// void HeapTimer::siftup_(size_t i) {
//     assert(i >= 0 && i < heap_.size());
//     size_t j = (i - 1) / 2; // i节点的父节点下标为（i - 1）/ 2,
//     // i节点的左右节点下标分别为 2 * i + 1 和 2 * i + 2
//     // 最小堆非叶子节点的值不大于左右节点的值
//     while(j >= 0) {
//         if(heap_[j] < heap_[i]) { break; } // 如果父节点已经小于当前节点，就不需要再上升了
//         //此时当前节点小于父节点，需要进行调整
//         SwapNode_(i, j);
//         i = j; // 此时i节点已经被交换至父节点（j）的位置，但i和j的下标值还没有交换，所以需要重新赋值
//         j = (i - 1) / 2; //新的父节点下标
//     }
// }

// bool HeapTimer::siftdown_(size_t index, size_t n) {
//     assert(index >= 0 && index < heap_.size());
//     assert(n >= 0 && n <= heap_.size());
//     size_t i = index;
//     size_t j = i * 2 + 1; // i的左节点
//     while(j < n) {
//         // 如果右节点小于左节点，后面就和右节点交换，否则就和左节点交换。和小的那个子节点交换位置
//         if(j + 1 < n && heap_[j + 1] < heap_[j]) j++; 
//         if(heap_[i] < heap_[j]) break; //如果当前节点已经小于最小的子节点，就不需要再下沉了
//         SwapNode_(i, j);
//         i = j;
//         j = i * 2 + 1; //新的左孩子节点的位置
//     }
//     return i > index; //如果i没有下沉，就返回false
// }

// void HeapTimer::del_(size_t index) {
    
//     assert(!heap_.empty() && index >= 0 && index < heap_.size());
    
//     size_t i = index;
//     size_t n = heap_.size() - 1;
//     assert(i <= n);
//     if(i < n) {
//         //如果i==n，就直接删除最后一个节点
//         SwapNode_(i, n); //把最后一个节点和当前要删除的节点交换
//         if(!siftdown_(i, n)) { //然后调整交换后的点的位置
//             siftup_(i);
//         }
//     }
    
//     ref_.erase(heap_.back());
//     heap_.pop_back();
// }

// void HeapTimer::SwapNode_(size_t i, size_t j) { 
//     assert(i >= 0 && i < heap_.size());
//     assert(j >= 0 && j < heap_.size());
//     std::swap(heap_[i], heap_[j]);
//     ref_[heap_[i]] = i;
//     ref_[heap_[j]] = j;
// } 

int main() {
    const char * str1 = "asdasd";
    


}










