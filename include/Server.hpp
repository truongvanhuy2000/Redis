#ifndef __ECHO__SERVER__
#define __ECHO__SERVER__
#include <stdint.h>
#include <iostream>
#include <vector>
#include <poll.h>
#include <unordered_map>
#include "timer.hpp"

#define k_max_msg 4096
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
class requestHandler;
class responseHandler;

class echoServer
{
    // Define constant here
    const static int k_max_arg = 1024;
    // User defined data
    timer globalTime;

    requestHandler *request;
    responseHandler *response;

    std::vector<std::string> requestList;
    std::vector<std::string> responseList;

    std::unordered_map<std::string, std::string> dataMap;

    std::vector<struct pollfd> fdArr;
    std::vector<struct Conn *> connArr;

    int serverInitialization();

    int acceptNewConnection(std::vector<Conn *> &fd2Conn, int fd);
    void connectionIO(Conn *conn);
    void closeConnection(Conn *conn);

    void timeOutHandler();
public:
    echoServer();
    ~echoServer();
    int startServer();

    friend class requestHandler;
    friend class responseHandler;
};
#endif