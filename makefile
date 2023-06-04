CXX ?= g++

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2

endif

server: ./timer/priority_timer.cpp ./threadpool/thread_pool_new.h ./lock/locker.h ./http/http_conn.cpp ./log/log.cpp ./config/config.cpp webserver_new.cpp  main.cpp
	$(CXX) -o server  $^ $(CXXFLAGS) -lpthread

clean:
	rm  -r server
