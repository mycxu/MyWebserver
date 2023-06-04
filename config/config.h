#ifndef CONFIG_H
#define CONFIG_H

#include "../webserver.h"
class Config {
public:
    Config();
    ~Config() {};

    int PORT;

    int OPT_LINGER;

    int TRIGMode;

    int thread_num;

    int CLOSE_LOG;

    int LOGWrite;
};

#endif