#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include "../lock/locker.h"
#include <sys/uio.h>



class http_conn {
public:
    static const int FILENAME_LEN = 200;        // �ļ�������󳤶�
    static const int READ_BUFFER_SIZE = 2048;   // ���������Ĵ�С
    static const int WRITE_BUFFER_SIZE = 1024;  // д�������Ĵ�С

    enum METHOD { GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT };
    enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };
    enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };
public:
    http_conn() {}
    ~http_conn() {}
public:
    //��ʼ���׽��ֵ�ַ�������ڲ������˽�з���init
    void init(int connfd, const sockaddr_in& client_addr, char *root, int TRIGMode);
    void close_conn();  // �ر�����
    void process(); // �����ͻ�������
    bool read();// ��������
    bool write();// ������д
    sockaddr_in * get_address() {
        return &m_address;
    }

public:
    static int m_epollfd;
    static int m_user_count;
private:
    void init();
    HTTP_CODE process_read();   //��m_read_buf��ȡ��������������
    bool process_write(HTTP_CODE ret);   //��m_write_bufд����Ӧ��������

    // ������һ�麯����process_read�����Է���HTTP����
    
    //��״̬�����������е�����������
    HTTP_CODE parse_request_line(char* text);
    //��״̬�����������е�����ͷ����
    HTTP_CODE parse_headers(char* text);
    //��״̬�����������е���������
    HTTP_CODE parse_content(char* text);
    //������Ӧ����
    HTTP_CODE do_request();

    //m_start_line���Ѿ��������ַ�
    //get_line���ڽ�ָ�����ƫ�ƣ�ָ��δ�������ַ�
    char* get_line() { return m_read_buf + m_start_line; }

    //��״̬����ȡһ�У������������ĵ���һ����
    LINE_STATUS parse_line();

    // ��һ�麯����process_write���������HTTPӦ��
    void unmap();

    //������Ӧ���ĸ�ʽ�����ɶ�Ӧ8�����֣����º�������do_request����
    bool add_response(const char* format, ...);
    bool add_content(const char* content);
    bool add_content_type();
    bool add_status_line(int status, const char* title);
    bool add_headers(int content_length);
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();
private:
    int m_sockfd;
    sockaddr_in m_address;

    char m_read_buf[READ_BUFFER_SIZE];    //�洢��ȡ������������
    int m_read_idx;                         //��������m_read_buf�����ݵ����һ���ֽڵ���һ��λ��
    int m_checked_idx;                      //m_read_buf��ȡ��λ��m_checked_idx
    int m_start_line;                       // ��ǰ���ڽ������е���ʼλ�� m_read_buf���Ѿ��������ַ�����

    CHECK_STATE m_check_state;              // ��״̬����ǰ������״̬
    METHOD m_method;                        // ���󷽷�

    char m_real_file[FILENAME_LEN];       // �ͻ������Ŀ���ļ�������·���������ݵ��� doc_root + m_url, doc_root����վ��Ŀ¼
    char* m_url;                            // �ͻ������Ŀ���ļ����ļ���
    char* m_version;                        // HTTPЭ��汾�ţ����ǽ�֧��HTTP1.1
    char* m_host;                           // ������
    int m_content_length;                   // HTTP�������Ϣ�ܳ���
    bool m_linger;                          // HTTP�����Ƿ�Ҫ�󱣳�����
    char *m_string; //存储请求头数据
    
    char m_write_buf[WRITE_BUFFER_SIZE];  //�洢��������Ӧ��������
    int m_write_idx;                        // д�������д����͵��ֽ��� ָʾbuffer�еĳ���
    char* m_file_address;                   // �ͻ������Ŀ���ļ���mmap���ڴ��е���ʼλ��
    struct stat m_file_stat;                // Ŀ���ļ���״̬��ͨ�������ǿ����ж��ļ��Ƿ���ڡ��Ƿ�ΪĿ¼���Ƿ�ɶ�������ȡ�ļ���С����Ϣ
    struct iovec m_iv[2];                   // ���ǽ�����writev��ִ��д���������Զ�������������Ա������m_iv_count��ʾ��д�ڴ���������
    int m_iv_count;
};




#endif