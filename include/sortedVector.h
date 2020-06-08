#pragma once

#include <vector>

/*
    /!\ Template needs definition of operator<
*/


template<typename T>
class SortedVector : public std::vector<T>
{
public:
    SortedVector(int maxSize);
    ~SortedVector() {};
    void push_sorted(const T &element);

private:
    const int _maxSize;
};


template<typename T>
SortedVector<T>::SortedVector(int maxSize) :
    _maxSize(maxSize)
{
}

template<typename T>
void SortedVector<T>::push_sorted(const T &element)
{
    if ((size() >= _maxSize) && !(element < back()))
        return;

    if (size() < _maxSize)
        push_back(element);

    auto pos = std::upper_bound(begin(), end() - 1, element);
    std::move_backward(pos, end() - 1, end());
    *pos = element;
}
