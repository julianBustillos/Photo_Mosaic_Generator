#pragma once
#include <iostream>
#include <io.h>


class OutputManager
{
public:
    static OutputManager& get()
    {
        static OutputManager instance;
        return instance;
    }

private:
    OutputManager() {};

public:
    OutputManager(OutputManager const&) = delete;
    void operator=(OutputManager const&) = delete;

public:
    inline void cstderr_silent();
    inline void cstderr_restore();

private:
    int _stderr = -1;
    FILE* _stream = nullptr;
};


inline void OutputManager::cstderr_silent()
{
    
    if (!_stream)
    {
        fflush(stderr);
        _stderr = _dup(_fileno(stderr));
        freopen_s(&_stream, "NUL:", "w", stderr);
    }
}

inline void OutputManager::cstderr_restore()
{
    if (_stream)
    {
        fflush(stderr);
        _dup2(_stderr, _fileno(stderr));
        _close(_stderr);
        _stream = nullptr;
        _stderr = -1;
    }
}

