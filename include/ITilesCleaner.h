#pragma once

#include "ITiles.h"


class ITilesCleaner
{
public:
    ITilesCleaner() {};
    ~ITilesCleaner() {};

public:
    virtual void clean(ITiles& tiles) const = 0;
};

