#include "config.h"

Config::Config(){
    //端口号,默认9006
    PORT = 9006;

    //触发组合模式,默认listenfd ET + connfd ET
    TRIGMode = 3;

    //优雅关闭链接，默认不使用
    OPT_LINGER = 0;

    //线程池内的线程数量,默认8
    thread_num = 8;

    CLOSE_LOG = 0;

    LOGWrite = 1;
}