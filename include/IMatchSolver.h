#pragma once

#include "Photo.h"
#include "ITiles.h"



class IMatchSolver
{
public:
    IMatchSolver(std::shared_ptr<const Photo> photo, int subdivisions) : _photo(photo), _subdivisions(subdivisions) {};
    virtual ~IMatchSolver() { _photo.reset(); };

public:
    virtual void solve(const ITiles& tiles) = 0;
    virtual const std::vector<int>& getMatchingTiles() const = 0;

protected:
    std::shared_ptr<const Photo> _photo;
    const int _subdivisions;
};
