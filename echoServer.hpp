#ifndef __ECHO__SERVER__
#define __ECHO__SERVER__
#include <stdint.h>
#include <iostream>
#include <vector>
#include <poll.h>
#include <unordered_map>
class echoServer
{
    // Define constant here
    const static size_t k_max_msg = 4096;
    const static int k_max_arg = 1024;
    // User defined data
    enum
    {
        RES_OK = 0,
        RES_ERR = 1,
        RES_NX = 2,
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
    };
    std::vector<std::string> requestList;
    std::vector<std::string> responseList;

    std::unordered_map<std::string, std::string> dataMap;

    std::vector<struct pollfd> fdArr;
    std::vector<struct Conn *> connArr;

    int acceptNewConnection(std::vector<Conn *> &fd2Conn, int fd);
    void connectionIO(Conn *conn);
    void requestState(Conn *conn);
    void responseState(Conn *conn);
    int requestProcess(Conn *conn);
    void sendResponse(Conn *conn);
    int doRequest();
    int doGet(std::vector<std::string> &cmd, std::string &resString);
    int doSet(std::vector<std::string> &cmd, std::string &resString);
    int doDel(std::vector<std::string> &cmd, std::string &resString);

public:
    echoServer() {}
    ~echoServer() {}
    int startServer();
    friend class helper;
};
#endif