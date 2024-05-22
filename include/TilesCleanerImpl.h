#pragma once

#include "ITilesCleaner.h"


class TilesCleanerImpl : public ITilesCleaner
{
public:
    virtual void clean(ITiles& tiles) const;
};

