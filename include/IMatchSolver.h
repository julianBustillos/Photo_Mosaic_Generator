#pragma once

#include "Photo.h"
#include "ITiles.h"



class IMatchSolver
{
public:
    //TODO : ADD COMMENT
    IMatchSolver(const Photo& photo, int subdivisions) : _photo(photo), _subdivisions(subdivisions) {};

    //TODO : ADD COMMENT
    virtual ~IMatchSolver() {};

    //TODO : ADD COMMENT
    virtual void solve(const ITiles& tiles) = 0;

    //TODO : ADD COMMENT
    virtual const std::vector<int>& getMatchingTiles() const = 0;

protected:
    const Photo& _photo;
    const int _subdivisions;
};
