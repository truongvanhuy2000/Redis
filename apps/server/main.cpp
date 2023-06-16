#include <iostream>
#include <redis/Server.hpp>

int main()
{
    echoServer newServer;
    newServer.startServer();
    return 0;
}