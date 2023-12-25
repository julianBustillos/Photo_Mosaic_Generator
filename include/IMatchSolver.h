#pragma once

#include "Photo.h"
#include "ITiles.h"



class IMatchSolver
{
public:
    //TODO : ADD COMMENT
    IMatchSolver(std::shared_ptr<const Photo> photo, int subdivisions) : _photo(photo), _subdivisions(subdivisions) {};

    //TODO : ADD COMMENT
    virtual ~IMatchSolver() { _photo.reset(); };

    //TODO : ADD COMMENT
    virtual void solve(const ITiles& tiles) = 0;

    //TODO : ADD COMMENT
    virtual const std::vector<int>& getMatchingTiles() const = 0;

protected:
    std::shared_ptr<const Photo> _photo;
    const int _subdivisions;
};
