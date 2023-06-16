#pragma once

#include <iostream>
#include <vector>
#include <string>

class helper
{
public:
    static int setFdToNonblocking(int fd);
    static int parseRequest(char *request, int len, std::vector<std::string> &output);
};
