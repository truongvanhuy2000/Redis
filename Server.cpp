#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <cstring>
#include <cassert>

#include "helper.hpp"
#include "Server.hpp"
#include "marco.hpp"

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
        requestState(conn);
        break;
    case STATE_RES:
        responseState(conn);
        break;
    default:
        return;
    }
}
void echoServer::requestState(Conn *conn)
{
    int receivedByte = 0;
    // std::cout << "Start request state" << std::endl;
    while (conn->state == STATE_REQ)
    {
        do
        {
            // Try to read the socket
            receivedByte = read(conn->fd, conn->rbuf, sizeof(conn->rbuf) - conn->rbuf_size);
            // Error code EINTR typically means that a system call was interrupted by a signal before it could complete its task
        } while (receivedByte < 0 && errno == EINTR);
        // The error code EAGAIN typically means that a non-blocking operation could not be completed immediately and that the caller should try again later
        if (errno == EAGAIN && receivedByte < 0)
        {
            // std::cerr << "EAGAIN error" << std::endl;
            break;
        }
        if (receivedByte < 0)
        {
            std::cerr << "read error" << std::endl;
            conn->state = STATE_END;
            break;
        }
        if (receivedByte == 0)
        {
            std::cout << "read EOF" << std::endl;
            conn->state = STATE_END;
            break;
        }
        conn->rbuf_size += receivedByte;
        if (requestProcess(conn))
        {
            // Do what each request say
            doRequest();
            conn->state = STATE_RES;
        }
    }
}
int echoServer::requestProcess(Conn *conn)
{
    int len = 0;
    char *dataPtr = (char *)conn->rbuf;
    std::string tempString;
    while (conn->rbuf_size >= 4)
    {
        len = *((int *)dataPtr);
        if (len > k_max_msg)
        {
            std::cerr << "exceed the maxium length of a data packet!!!" << std::endl;
            conn->state = STATE_END;
            return 0;
        }
        if (conn->rbuf_size - 4 < len)
        {
            std::cout << "haven't received enough byte, keep reading" << std::endl;
            return 0;
        }
        // Create buffer for the request and copy data from the read buffer to it

        tempString = std::string(dataPtr + 4, len);
        // std::cout << "Client say: " << tempString << std::endl;

        // Push the new request to the request list
        requestList.push_back(tempString);

        conn->rbuf_size = conn->rbuf_size - len - 4;
        // Move to the next data point
        dataPtr += len + 4;
    }
    return 1;
}
int echoServer::doRequest()
{
    std::vector<std::string> command;
    int respondCode = 0;
    std::string respondString;
    for (std::string request : requestList)
    {
        helper::parseRequest(request.data(), request.size(), command);
        respondString.clear();
        if (command.size() == 1 && command[0] == "keys")
        {
            doKeys(command, respondString);
        }
        else if (command.size() == 2 && command[0] == "get")
        {
            doGet(command, respondString);
        }
        else if (command.size() == 3 && command[0] == "set")
        {
            doSet(command, respondString);
        }
        else if (command.size() == 2 && command[0] == "del")
        {
            doDel(command, respondString);
        }
        else
        {
            outErr(respondString, ERR_UNKNOWN, "Unknown cmd");
        }
        // Insert the respond of the string
        responseList.push_back(respondString);
    }
    requestList.clear();
    return 1;
}
void echoServer::doKeys(std::vector<std::string> &cmd, std::string &resString)
{
    outArr(resString, dataMap.size());
    for (auto &pair : dataMap)
    {
        std::string key = pair.first;
        outStr(resString, key);
    }
}
void echoServer::doGet(std::vector<std::string> &cmd, std::string &resString)
{
    if (cmd.size() < 2)
    {
        std::cerr << "doGet() Not enough command" << std::endl;
        return outErr(resString, ERR_UNKNOWN, "Wrong cmd format");
    }
    if (dataMap.count(cmd[1]) <= 0)
    {
        std::cerr << "doGet() Element not exist in the data map" << std::endl;
        return outNull(resString);
    }
    return outStr(resString, dataMap[cmd[1]]);
}
void echoServer::doSet(std::vector<std::string> &cmd, std::string &resString)
{
    if (cmd.size() < 3)
    {
        std::cerr << "doSet() Not enough command" << std::endl;
        return outErr(resString, ERR_UNKNOWN, "Wrong cmd format");
    }
    dataMap[cmd[1]] = cmd[2];
    return outInt(resString, 1);
}
// THis will handle the del command recv from client
void echoServer::doDel(std::vector<std::string> &cmd, std::string &resString)
{
    if (cmd.size() < 2)
    {
        std::cerr << "doDel() Not enough command" << std::endl;
        return outErr(resString, ERR_UNKNOWN, "Wrong cmd format");
    }
    if (dataMap.count(cmd[1]) <= 0)
    {
        std::cerr << "doDel() Element not exist in the data map" << std::endl;
        return outInt(resString, 0);
    }
    dataMap.erase(cmd[1]);
    return outInt(resString, 1);
}
void echoServer::outNull(std::string &resString)
{
    resString.push_back(SER_NIL);
}
void echoServer::outStr(std::string &resString, std::string &data)
{
    resString.push_back(SER_STR);
    int len = data.size();
    resString.append((char *)&len, 4);
    resString.append(data);
}
void echoServer::outInt(std::string &resString, int64_t val)
{
    resString.push_back(SER_INT);
    resString.append((char *)&val, 8);
}
void echoServer::outErr(std::string &resString, int errorCode, const std::string &errorMsg)
{
    resString.push_back(SER_ERR);
    resString.append((char *)&errorCode, 4);
    int msgSize = errorMsg.size();
    resString.append((char *)&msgSize, 4);
    resString.append(errorMsg);
}
void echoServer::outArr(std::string &resString, int size)
{
    resString.push_back(SER_ARR);
    resString.append((char *)&size, 4);
}
void echoServer::responseState(Conn *conn)
{
    int len = 0;
    // std::cout << "Start response process" << std::endl;
    for (std::string response : responseList)
    {
        len = response.size();
        conn->wbuf_size = len + 4;

        // Send respond command to client
        memcpy(conn->wbuf, &len, 4);
        memcpy(conn->wbuf + 4, response.data(), len);

        sendResponse(conn);
    }
    responseList.clear();
}
void echoServer::sendResponse(Conn *conn)
{
    int sentByte = 0;
    while (1)
    {
        do
        {
            sentByte = write(conn->fd, &conn->wbuf[conn->wbuf_sent], conn->wbuf_size - conn->wbuf_sent);
        } while (sentByte < 0 && errno == EINTR);

        if (errno == EAGAIN && sentByte < 0)
        {
            break;
        }

        if (sentByte < 0)
        {
            std::cerr << "write error" << std::endl;
            conn->state = STATE_END;
            break;
        }
        conn->wbuf_sent = sentByte;

        if (conn->wbuf_sent == conn->wbuf_size)
        {
            // std::cout << "Sent to client successfully" << std::endl;
            conn->state = STATE_REQ;
            // Sent successfully
            conn->wbuf_sent = 0;
            conn->wbuf_size = 0;
            // Deallocate the data
            memset(conn->wbuf, 0, conn->wbuf_size);
            break;
        }
    }
}