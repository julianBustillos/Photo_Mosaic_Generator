#pragma once

#include "ITiles.h"


class ITilesCleaner
{
public:
    //TODO : ADD COMMENT
    virtual void clean(ITiles& tiles) const = 0;
};

