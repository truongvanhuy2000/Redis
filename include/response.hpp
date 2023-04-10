#ifndef __RESPONSE__HPP
#define __RESPONSE__HPP
#include <vector>
#include <string>
#include "Server.hpp"

class responseHandler
{
    echoServer &server;
public:
    responseHandler(echoServer &server) : server(server) {}
    void responseState(Conn *conn);
    void sendResponse(Conn *conn);
};
#endif