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
    void push_sorted(const T& element);

private:
    const int _maxSize;
};


template<typename T>
SortedVector<T>::SortedVector(int maxSize) :
    _maxSize(maxSize)
{
}

template<typename T>
void SortedVector<T>::push_sorted(const T& element)
{
    if ((this->size() >= _maxSize) && !(element < this->back()))
        return;

    if (this->size() < _maxSize)
        this->push_back(element);

    auto pos = std::upper_bound(this->begin(), this->end() - 1, element);
    std::move_backward(pos, this->end() - 1, this->end());
    *pos = element;
}
