#include "sortedVector.h"


template<typename T>
inline SortedVector<T>::SortedVector(int maxSize) :
    _maxSize(maxSize)
{
}

template<typename T>
void SortedVector<T>::push_sorted(T element)
{
    if ((size() >= _maxSize) && (element >= back()))
        return;

    if (size() < _maxSize) 
        v.push_back(element);

    auto pos = std::upper_bound(begin(), end() - 1, element);
    std::move_backward(pos, end() - 1, pos + 1);
    *pos = elem;
}
