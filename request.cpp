#include "include/request.hpp"
#include "include/helper.hpp"
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

void requestHandler::requestState(Conn *conn)
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

int requestHandler::requestProcess(Conn *conn)
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
        server.requestList.push_back(tempString);

        conn->rbuf_size = conn->rbuf_size - len - 4;
        // Move to the next data point
        dataPtr += len + 4;
    }
    return 1;
}
int requestHandler::doRequest()
{
    std::vector<std::string> command;
    int respondCode = 0;
    std::string respondString;
    for (std::string request : server.requestList)
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
        server.responseList.push_back(respondString);
    }
    server.requestList.clear();
    return 1;
}
void requestHandler::doKeys(std::vector<std::string> &cmd, std::string &resString)
{
    outArr(resString, server.dataMap.size());
    for (auto &pair : server.dataMap)
    {
        std::string key = pair.first;
        outStr(resString, key);
    }
}
void requestHandler::doGet(std::vector<std::string> &cmd, std::string &resString)
{
    if (cmd.size() < 2)
    {
        std::cerr << "doGet() Not enough command" << std::endl;
        return outErr(resString, ERR_UNKNOWN, "Wrong cmd format");
    }
    if (server.dataMap.count(cmd[1]) <= 0)
    {
        std::cerr << "doGet() Element not exist in the data map" << std::endl;
        return outNull(resString);
    }
    return outStr(resString, server.dataMap[cmd[1]]);
}
void requestHandler::doSet(std::vector<std::string> &cmd, std::string &resString)
{
    if (cmd.size() < 3)
    {
        std::cerr << "doSet() Not enough command" << std::endl;
        return outErr(resString, ERR_UNKNOWN, "Wrong cmd format");
    }
    server.dataMap[cmd[1]] = cmd[2];
    return outInt(resString, 1);
}
// THis will handle the del command recv from client
void requestHandler::doDel(std::vector<std::string> &cmd, std::string &resString)
{
    if (cmd.size() < 2)
    {
        std::cerr << "doDel() Not enough command" << std::endl;
        return outErr(resString, ERR_UNKNOWN, "Wrong cmd format");
    }
    if (server.dataMap.count(cmd[1]) <= 0)
    {
        std::cerr << "doDel() Element not exist in the data map" << std::endl;
        return outInt(resString, 0);
    }
    server.dataMap.erase(cmd[1]);
    return outInt(resString, 1);
}
void requestHandler::outNull(std::string &resString)
{
    resString.push_back(SER_NIL);
}
void requestHandler::outStr(std::string &resString, std::string &data)
{
    resString.push_back(SER_STR);
    int len = data.size();
    resString.append((char *)&len, 4);
    resString.append(data);
}
void requestHandler::outInt(std::string &resString, int64_t val)
{
    resString.push_back(SER_INT);
    resString.append((char *)&val, 8);
}
void requestHandler::outErr(std::string &resString, int errorCode, const std::string &errorMsg)
{
    resString.push_back(SER_ERR);
    resString.append((char *)&errorCode, 4);
    int msgSize = errorMsg.size();
    resString.append((char *)&msgSize, 4);
    resString.append(errorMsg);
}
void requestHandler::outArr(std::string &resString, int size)
{
    resString.push_back(SER_ARR);
    resString.append((char *)&size, 4);
}