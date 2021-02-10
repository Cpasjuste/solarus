#pragma once


#include <vector>
#include <variant>

namespace Solarus {

/// Provides an indexed free list with constant-time removals from anywhere
/// in the list without invalidating indices. T must be trivially constructible
/// and destructible.
template <class T>
class FreeList
{
public:
    /// Creates a new free list.
    FreeList();

    /// Inserts an element to the free list and returns an index to it.
    int insert(const T& element);

    // Removes the nth element from the free list.
    void erase(int n);

    // Removes all elements from the free list.
    void clear();

    // Returns the range of valid indices.
    int range() const;

    // Returns the nth element.
    T& operator[](int n);

    // Returns the nth element.
    const T& operator[](int n) const;

    size_t size() const;

private:
    /*union FreeElement
    {
        T element;
        int next;
    };*/
    using FreeElement = std::variant<T, int>;
    std::vector<FreeElement> data;
    int first_free;
    size_t count;
};

template <class T>
FreeList<T>::FreeList(): first_free(-1), count(0)
{
}

template <class T>
int FreeList<T>::insert(const T& element)
{
    count++;
    if (first_free != -1)
    {
        const int index = first_free;
        first_free = std::get<1>(data[first_free]);
        data[index] = element;
        return index;
    }
    else
    {
        data.push_back(element);
        return static_cast<int>(data.size() - 1);
    }
}

template <class T>
void FreeList<T>::erase(int n)
{
    count--;
    data[n] = first_free;
    first_free = n;
}

template <class T>
void FreeList<T>::clear()
{
    data.clear();
    first_free = -1;
}

template <class T>
int FreeList<T>::range() const
{
    return static_cast<int>(data.size());
}

template <class T>
T& FreeList<T>::operator[](int n)
{
    auto& val = data[n];
    return std::get<0>(val);
}

template <class T>
const T& FreeList<T>::operator[](int n) const
{
    return std::get<0>(data[n]);
}

template <class T>
size_t FreeList<T>::size() const {
  return count;
}
}
