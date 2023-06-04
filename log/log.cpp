#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include "log.h"
#include <pthread.h>
using namespace std;

Log::Log()
{
    m_count = 0;
    m_is_async = false;
}

Log::~Log()
{
    if (m_fp != NULL)
    {
        fclose(m_fp);
    }
}

//异步需要设置阻塞队列的长度，同步不需要设置
bool Log::init(const char *file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size)
{
    //如果设置了max_queue_size,则设置为异步
    if (max_queue_size >= 1)
    {
        //设置写入方式flag
        m_is_async = true;

        //创建并设置阻塞队列长度
        m_log_queue = new block_queue<string>(max_queue_size);
        pthread_t tid;

        //flush_log_thread为回调函数,这里表示创建线程异步写日志
        pthread_create(&tid, NULL, flush_log_thread, NULL);
    }

    m_close_log = close_log;

    //输出内容的长度
    m_log_buf_size = log_buf_size; //日志缓冲区大小
    m_buf = new char[m_log_buf_size];
    memset(m_buf, '\0', m_log_buf_size);
    m_split_lines = split_lines; //日志最大行数

    time_t t = time(NULL);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    //从后往前找到第一个/的位置
    const char *p = strrchr(file_name, '/'); // char *strrchr(const char *str, int c) 在参数 str 所指向的字符串中搜索最后一次出现字符 c（一个无符号字符）的位置
    char log_full_name[291] = {0};

    //相当于自定义日志名
    //若输入的文件名没有/，则直接将时间+文件名作为日志名
    if (p == NULL)
    {
        //函数原型：int snprintf(char* dest_str,size_t size,const char* format,...);
        //将可变个参数(...)按照format格式化成字符串，然后将其复制到str中。
        //(1) 如果格式化后的字符串长度 < size，则将此字符串全部复制到str中，并给其后添加一个字符串结束符('\0')；
        //(2) 如果格式化后的字符串长度 >= size，则只将其中的(size-1)个字符复制到str中，并给其后添加一个字符串结束符('\0')，
        //返回值为欲写入的字符串长度
        snprintf(log_full_name, 290, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
    }
    else
    {   
        //将/的位置向后移动一个位置，然后复制到logname中
        //p - file_name + 1是文件所在路径文件夹的长度
        //dirname相当于./
        strcpy(log_name, p + 1);
        strncpy(dir_name, file_name, p - file_name + 1); // p为filename 中最后一个/的地址
        snprintf(log_full_name, 290, "%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
    }

    m_today = my_tm.tm_mday;
    
    m_fp = fopen(log_full_name, "a");
    //a: 以追加方式打开写文件 
    //如果文件不存在, 则创建文件 
    //如果文件存在, 则新写入的数据会被追加到文件末尾, 文件原来的数据会被保留
    if (m_fp == NULL)
    {
        return false;
    }

    return true;
}

void Log::write_log(int level, const char *format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char s[16] = {0};
    switch (level)
    {
    case 0:
        strcpy(s, "[debug]:");
        break;
    case 1:
        strcpy(s, "[info]:");
        break;
    case 2:
        strcpy(s, "[warn]:");
        break;
    case 3:
        strcpy(s, "[erro]:");
        break;
    default:
        strcpy(s, "[info]:");
        break;
    }

    //写入一个log，对m_count++, m_split_lines最大行数
    m_mutex.lock();
    m_count++;

    //日志不是今天或写入的日志行数是最大行的倍数
    //m_split_lines为最大行数
    if (m_today != my_tm.tm_mday || m_count % m_split_lines == 0) //everyday log
    {
        
        char new_log[291] = {0};
        fflush(m_fp);
        fclose(m_fp);
        char tail[16] = {0};
       
       //格式化日志名中的时间部分
        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);
       
        //如果是时间不是今天,则创建今天的日志，更新m_today和m_count
        if (m_today != my_tm.tm_mday)
        {
            snprintf(new_log, 290, "%s%s%s", dir_name, tail, log_name);
            m_today = my_tm.tm_mday;
            m_count = 0;
        }
        else
        {
            //超过了最大行，在之前的日志名基础上加后缀, m_count/m_split_lines
            snprintf(new_log, 290, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_split_lines);
        }
        m_fp = fopen(new_log, "a");
    }
 
    m_mutex.unlock();

    va_list valst;
    //将传入的format参数赋值给valst，便于格式化输出
    va_start(valst, format);

    string log_str;
    m_mutex.lock();

    //写入内容格式：时间 + 内容
    //时间格式化，snprintf成功返回写字符的总数，其中不包括结尾的null字符
    int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
    
    //内容格式化，用于向字符串中打印数据、数据格式用户自定义，返回写入到字符数组str中的字符个数(不包含终止符)
    int m = vsnprintf(m_buf + n, m_log_buf_size - n - 1, format, valst);
    m_buf[n + m] = '\n';
    m_buf[n + m + 1] = '\0';
    log_str = m_buf;

    m_mutex.unlock();

    //若m_is_async为true表示异步，默认为同步
    //若异步,则将日志信息加入阻塞队列,同步则加锁向文件中写s
    if (m_is_async && !m_log_queue->full())
    {
        m_log_queue->push(log_str);
    }
    else
    {
        m_mutex.lock();
        fputs(log_str.c_str(), m_fp);
        m_mutex.unlock();
    }

    va_end(valst);
}

void Log::flush(void)
{
    m_mutex.lock();
    //强制刷新写入流缓冲区
    fflush(m_fp);
    m_mutex.unlock();
}
