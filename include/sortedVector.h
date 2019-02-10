#pragma once

#include <vector>


template<typename T> class SortedVector : public std::vector<T>
{
public:
    SortedVector(int maxSize);
    ~SortedVector() {};

private:
    void push_sorted(T element);

private:
    const int _maxSize;
};
