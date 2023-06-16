#pragma once

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