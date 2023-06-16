#pragma once

#include <vector>
#include <string>
#include "Server.hpp"

class requestHandler
{
    echoServer &server;

public:
    requestHandler(echoServer &server) : server(server) {}
    int requestProcess(Conn *conn);
    int doRequest();
    
    void requestState(Conn *conn);
    void doKeys(std::vector<std::string> &cmd, std::string &resString);
    void doGet(std::vector<std::string> &cmd, std::string &resString);
    void doSet(std::vector<std::string> &cmd, std::string &resString);
    void doDel(std::vector<std::string> &cmd, std::string &resString);

    void outNull(std::string &resString);
    void outStr(std::string &resString, std::string &data);
    void outInt(std::string &resString, int64_t val);
    void outErr(std::string &resString, int errorCode, const std::string &errorMsg);
    void outArr(std::string &resString, int size);
};