#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <signal.h>
#include <assert.h>

#include "./threadpool/thread_pool.h"
#include "./lock/locker.h"
#include "./http/http_conn.h"
#include "config.h"

int main(int argc, char* argv[]) {
    Config config;

    webserver server;

    server.init(config.PORT, config.OPT_LINGER, config.TRIGMode, config.thread_num, config.CLOSE_LOG, config.LOGWrite);

    server.log_write();

    server.thread_pool();

    server.trig_mode();

    server.eventListen();

    server.eventLoop();

    return 0;

}