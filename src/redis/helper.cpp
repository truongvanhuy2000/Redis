#include "redis/helper.hpp"

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

int helper::setFdToNonblocking(int fd)
{
    // Set file discriptor to be non blocking
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno)
    {
        std::cerr << "fcntl error" << std::endl;
        return 0;
    }

    flags |= O_NONBLOCK;

    errno = 0;
    (void)fcntl(fd, F_SETFL, flags);
    if (errno)
    {
        std::cerr << "fcntl error" << std::endl;
        return 0;
    }
    return 1;
}
int helper::parseRequest(char *request, int len, std::vector<std::string> &output)
{
    if (len < 4)
    {
        return 0;
    }
    int nstr = *((int *)request);

    if (nstr > 1023)
    {
        std::cerr << "Parse Request: exceed maximum arg length" << std::endl;
        return 0;
    }

    int requestSize = 0;
    char *dataPtr = request + 4; // Move pass the nstr parameter
    std::string temp;
    while (nstr--)
    {
        requestSize = *((int *)dataPtr);
        if (requestSize + 4 > len)
        {
            std::cerr << "Parse Request: exceed maximum msg length" << std::endl;
            return 0;
        }
        temp = std::string(dataPtr + 4, requestSize);
        output.push_back(temp);

        dataPtr += requestSize + 4;
    }
    return 1;
}