#ifndef HELPER__HPP___
#define HELPER__HPP___
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <string>

class helper
{
public:
    static int setFdToNonblocking(int fd);
    static int parseRequest(char *request, int len, std::vector<std::string> &output);
};

#endif