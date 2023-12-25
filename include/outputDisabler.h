#pragma once

#include <iostream>


class OutputDisabler
{
public:
    void start()
    {
        freopen_s(&_stream, "nul", "w", stderr);
    }

    void end()
    {
        freopen_s(&_stream, "CON", "w", stderr);
    }

private:
    FILE* _stream;
};

