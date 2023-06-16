#include "redis/response.hpp"
#include "redis/helper.hpp"

#include <cstring>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

void responseHandler::responseState(Conn *conn)
{
    int len = 0;
    // std::cout << "Start response process" << std::endl;
    for (std::string response : server.responseList)
    {
        len = response.size();
        conn->wbuf_size = len + 4;

        // Send respond command to client
        memcpy(conn->wbuf, &len, 4);
        memcpy(conn->wbuf + 4, response.data(), len);

        sendResponse(conn);
    }
    server.responseList.clear();
}
void responseHandler::sendResponse(Conn *conn)
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