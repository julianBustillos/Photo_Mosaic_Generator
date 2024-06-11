#pragma once

#include "Photo.h"
#include "ITiles.h"



class IMatchSolver
{
public:
    IMatchSolver(int subdivisions) : _subdivisions(subdivisions) {};
    virtual ~IMatchSolver() {};

public:
    virtual void solve(const ITiles& tiles) = 0;
    virtual const std::vector<int>& getMatchingTiles() const = 0;

protected:
    const int _subdivisions;
};
