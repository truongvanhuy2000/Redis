#include "redis/helper.hpp"
#include "redis/Server.hpp"
#include "redis/marco.hpp"
#include "redis/request.hpp"
#include "redis/response.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <cstring>
#include <cassert>

echoServer::echoServer()
{
    request = new requestHandler(*this);
    response = new responseHandler(*this);
}
echoServer::~echoServer()
{
    delete request;
}
int echoServer::startServer()
{
    std::cout << "Starting Server" << std::endl;
    // Start the global timer
    globalTime.initTimer();
    // Init the server
    int fd = serverInitialization();
    while (1)
    {
        fdArr.clear();
        struct pollfd pfd = {fd, POLLIN, 0};
        fdArr.push_back(pfd);

        // Basically check if there any connection to pull
        for (Conn *conn : connArr)
        {
            if (!conn)
            {
                continue;
            }
            struct pollfd pfd;
            pfd.fd = conn->fd;
            pfd.events = (conn->state == STATE_REQ) ? POLLIN : POLLOUT;
            // This indicates that the file descriptor associated with pfd should also be monitored for errors.
            // When the poll() system call detects an error on the file descriptor, it will set the revents member of pfd to include the POLLERR flag.
            pfd.events |= POLLERR;
            fdArr.push_back(pfd);
            // std::cout << "Connection: " << conn->fd << std::endl;
        }

        int timeOut = globalTime.getRemainingInterval();
        if (poll(fdArr.data(), fdArr.size(), timeOut) < 0)
        {
            std::cerr << "Polling error" << std::endl;
            return 0;
        }
        // Process active connection
        for (int i = 1; i < fdArr.size(); i++)
        {
            if (!fdArr[i].revents)
            {
                continue;
            }
            Conn *conn = connArr[fdArr[i].fd];
            connectionIO(conn);
            if (conn->state == STATE_END)
            {
                closeConnection(conn);
            }
        }
        timeOutHandler();
        if (fdArr[0].revents)
        {
            // Accept new connection if fd is active
            acceptNewConnection(connArr, fdArr[0].fd);
        }
    }
}
void echoServer::closeConnection(Conn *conn)
{
    // CLose the connection if client disconnect
    std::cout << "close a connection on: " << conn->fd << std::endl;
    connArr[conn->fd] = NULL;
    close(conn->fd);
    globalTime.deleteTimeslot(conn->timeInstance);
    delete conn;
}
void echoServer::timeOutHandler()
{
    while (!globalTime.emptyTimeSLot())
    {
        timeSlot *instance = globalTime.process_timers();
        if (!instance)
        {
            break;
        }
        Conn *conn = container_of(instance, Conn, timeInstance);
        closeConnection(conn);
    }
}
int echoServer::serverInitialization()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        std::cerr << "cant create socket" << std::endl;
        return 0;
    }
    int optionVal = 1;
    // Set the option of socket to be reuse
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optionVal, sizeof(optionVal));
    // Bind socket
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = ntohs(1234);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (const sockaddr *)&serverAddress, sizeof(serverAddress)) != 0)
    {
        std::cerr << "Cant bind socket" << std::endl;
        return 0;
    }

    // Listen to the damn port, allow maxium connection
    if (listen(fd, SOMAXCONN))
    {
        std::cerr << "Cant bind socket" << std::endl;
        return 0;
    }
    // Set the port to nonblocking mode
    helper::setFdToNonblocking(fd);
    return fd;
}
int echoServer::acceptNewConnection(std::vector<Conn *> &fd2Conn, int fd)
{
    sockaddr_in clientAddress = {};
    int sockLen = sizeof(clientAddress);
    // Accept new client connection
    int connFd = accept(fd, (sockaddr *)&clientAddress, (socklen_t *)&sockLen);
    if (connFd < 0)
    {
        std::cerr << "accept client error" << std::endl;
        return 0;
    }
    helper::setFdToNonblocking(connFd);
    Conn *newConn = new Conn();
    if (!newConn)
    {
        std::cerr << "allocate error bitch" << std::endl;
        return 0;
    }

    newConn->fd = connFd;
    newConn->state = STATE_REQ; // Client send request
    // Put the new connection into the vector
    if (newConn->fd >= fd2Conn.size())
    {
        fd2Conn.resize(newConn->fd + 1);
    }
    fd2Conn[newConn->fd] = newConn;
    // Initialize the time where its start
    globalTime.insertTimeslot(newConn->timeInstance);
    std::cout << "New client connection: " << newConn->fd << std::endl;

    for (int i = 0; i < fd2Conn.size(); i++)
    {
        if (!fd2Conn[i])
        {
            continue;
        }
    }
    return 1;
}
void echoServer::connectionIO(Conn *conn)
{
    // Update the time slot if there's any IO happen on the connection
    globalTime.timeslotUpdate(conn->timeInstance);
    switch (conn->state)
    {
    case STATE_REQ:
        request->requestState(conn);
        break;
    case STATE_RES:
        response->responseState(conn);
        break;
    default:
        return;
    }
}

