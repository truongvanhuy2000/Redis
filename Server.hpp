#ifndef __ECHO__SERVER__
#define __ECHO__SERVER__
#include <stdint.h>
#include <iostream>
#include <vector>
#include <poll.h>
#include <unordered_map>
#include "timer.hpp"

class echoServer
{
    // Define constant here
    const static size_t k_max_msg = 4096;
    const static int k_max_arg = 1024;
    // User defined data
    enum 
    {
        ERR_UNKNOWN = 1,
        ERR_2BIG = 2,
    };
    enum 
    {
        SER_NIL = 0,
        SER_ERR = 1,
        SER_STR = 2,
        SER_INT = 3,
        SER_ARR = 4,
    };
    enum
    {
        STATE_REQ = 0,
        STATE_RES = 1,
        STATE_END = 2, // mark the connection for deletion
    };
    struct Conn
    {
        int fd = -1;
        uint32_t state = 0; // either STATE_REQ or STATE_RES
        // buffer for reading
        size_t rbuf_size = 0;
        uint8_t rbuf[4 + k_max_msg];
        // buffer for writing
        size_t wbuf_size = 0;
        size_t wbuf_sent = 0;
        uint8_t wbuf[4 + k_max_msg];
        // Each connection have it own time slot instance
        timeSlot timeInstance;
    };
    timer globalTime;

    std::vector<std::string> requestList;
    std::vector<std::string> responseList;

    std::unordered_map<std::string, std::string> dataMap;

    std::vector<struct pollfd> fdArr;
    std::vector<struct Conn *> connArr;
    int serverInitialization();

    int acceptNewConnection(std::vector<Conn *> &fd2Conn, int fd);
    void connectionIO(Conn *conn);
    
    void requestState(Conn *conn);
    void responseState(Conn *conn);
    int requestProcess(Conn *conn);
    void sendResponse(Conn *conn);
    int doRequest();

    void doKeys(std::vector<std::string> &cmd, std::string &resString);
    void doGet(std::vector<std::string> &cmd, std::string &resString);
    void doSet(std::vector<std::string> &cmd, std::string &resString);
    void doDel(std::vector<std::string> &cmd, std::string &resString);

    void outNull(std::string &resString);
    void outStr(std::string &resString, std::string &data);
    void outInt(std::string &resString, int64_t val);
    void outErr(std::string &resString, int errorCode, const std::string &errorMsg);
    void outArr(std::string &resString, int size);
public:
    echoServer() {}
    ~echoServer() {}
    int startServer();
    friend class helper;
};
#endif