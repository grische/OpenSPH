#pragma once

/// Base classes useful for iterating and viewing all containers.
/// Pavel Sevecek 2016
/// sevecek at sirrah.troja.mff.cuni.cz

#include "core/Traits.h"
#include "geometry/Vector.h"

NAMESPACE_SPH_BEGIN

/// Simple (forward) iterator. Can be used with STL algorithms.
template <typename T, typename TCounter = Size>
class Iterator {
    template <typename, typename>
    friend class Array;
    template <typename, int, typename>
    friend class StaticArray;
    template <typename, typename>
    friend class ArrayView;

protected:
    using TValue = typename UnwrapReferenceType<T>::Type;

    T* data;
#ifdef DEBUG
    const T *begin, *end;
#endif

#ifndef DEBUG
    Iterator(T* data)
        : data(data) {}
#endif

    Iterator(T* data, const T* UNUSED_IN_RELEASE(begin), const T* UNUSED_IN_RELEASE(end))
        : data(data)
#ifdef DEBUG
        , begin(begin)
        , end(end)
#endif
    {
    }


public:
    Iterator() = default;

    const TValue& operator*() const {
        ASSERT(data >= begin);
        ASSERT(data < end);
        return *data;
    }
    TValue& operator*() {
        ASSERT(data >= begin);
        ASSERT(data < end);
        return *data;
    }
#ifdef DEBUG
    Iterator operator+(const TCounter n) const {
        return Iterator(data + n, begin, end);
    }
    Iterator operator-(const TCounter n) const {
        return Iterator(data - n, begin, end);
    }
#else
    Iterator operator+(const TCounter n) const {
        return Iterator(data + n);
    }
    Iterator operator-(const TCounter n) const {
        return Iterator(data - n);
    }
#endif
    void operator+=(const TCounter n) {
        data += n;
    }
    void operator-=(const TCounter n) {
        data -= n;
    }
    Iterator& operator++() {
        ++data;
        return *this;
    }
    Iterator operator++(int) {
        Iterator tmp(*this);
        operator++();
        return tmp;
    }
    Iterator& operator--() {
        --data;
        return *this;
    }
    Iterator operator--(int) {
        Iterator tmp(*this);
        operator--();
        return tmp;
    }
    Size operator-(const Iterator& iter) const {
        ASSERT(data >= iter.data);
        return data - iter.data;
    }
    bool operator<(const Iterator& iter) const {
        return data < iter.data;
    }
    bool operator>(const Iterator& iter) const {
        return data > iter.data;
    }
    bool operator<=(const Iterator& iter) const {
        return data <= iter.data;
    }
    bool operator>=(const Iterator& iter) const {
        return data >= iter.data;
    }
    bool operator==(const Iterator& iter) const {
        return data == iter.data;
    }
    bool operator!=(const Iterator& iter) const {
        return data != iter.data;
    }


    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = Size;
    using pointer = T*;
    using reference = T&;
};

/// Reverse iterator.
template <typename T, typename TCounter = Size>
class ReverseIterator {
protected:
    Iterator<T, Size> iter;

public:
    ReverseIterator() = default;

    ReverseIterator(Iterator<T, Size> iter)
        : iter(iter) {}

    decltype(auto) operator*() const {
        return *iter;
    }
    decltype(auto) operator*() {
        return *iter;
    }
    ReverseIterator& operator++() {
        --iter;
        return *this;
    }
    ReverseIterator operator++(int) {
        ReverseIterator tmp(*this);
        operator++();
        return tmp;
    }
    ReverseIterator& operator--() {
        ++iter;
        return *this;
    }
    ReverseIterator operator--(int) {
        ReverseIterator tmp(*this);
        operator--();
        return tmp;
    }
    bool operator==(const ReverseIterator& other) const {
        return iter == other.iter;
    }
    bool operator!=(const ReverseIterator& other) const {
        return iter != other.iter;
    }
};

template <typename T, typename TCounter>
ReverseIterator<T, TCounter> reverseIterator(const Iterator<T, TCounter> iter) {
    return ReverseIterator<T, TCounter>(iter);
}


/// Object providing safe access to continuous memory of data, useful to write generic code that can be used
/// with any kind of storage where the data are stored consecutively in memory.
template <typename T, typename TCounter = Size>
class ArrayView {
private:
    using StorageType = typename WrapReferenceType<T>::Type;

    StorageType* data;
    TCounter actSize = 0;

public:
    ArrayView()
        : data(nullptr) {}

    ArrayView(StorageType* data, TCounter size)
        : data(data)
        , actSize(size) {}

    ArrayView(const ArrayView& other)
        : data(other.data)
        , actSize(other.actSize) {}

    /// Move constructor
    ArrayView(ArrayView&& other) {
        std::swap(this->data, other.data);
        std::swap(this->actSize, other.actSize);
    }

    /// Explicitly initialize to nullptr
    ArrayView(std::nullptr_t)
        : data(nullptr) {}

    /// Copy operator
    ArrayView& operator=(const ArrayView& other) {
        this->data = other.data;
        this->actSize = other.actSize;
        return *this;
    }

    /// Move operator
    ArrayView& operator=(ArrayView&& other) {
        this->data = other.data;
        this->actSize = other.actSize;
        return *this;
    }

    /// Implicit conversion to const version
    operator ArrayView<const T, TCounter>() {
        return ArrayView<const T, TCounter>(data, actSize);
    }

    Iterator<StorageType, TCounter> begin() {
        return Iterator<StorageType, TCounter>(data, data, data + actSize);
    }

    Iterator<const StorageType, TCounter> begin() const {
        return Iterator<const StorageType, TCounter>(data, data, data + actSize);
    }

    Iterator<StorageType, TCounter> end() {
        return Iterator<StorageType, TCounter>(data + actSize, data, data + actSize);
    }

    Iterator<const StorageType, TCounter> end() const {
        return Iterator<const StorageType, TCounter>(data + actSize, data, data + actSize);
    }

    INLINE T& operator[](const TCounter idx) {
        ASSERT(unsigned(idx) < unsigned(actSize));
        return data[idx];
    }

    INLINE const T& operator[](const TCounter idx) const {
        ASSERT(unsigned(idx) < unsigned(actSize));
        return data[idx];
    }

    INLINE TCounter size() const {
        return this->actSize;
    }

    INLINE bool empty() const {
        return this->actSize == 0;
    }

    /// Returns a subset of the arrayview.
    INLINE ArrayView subset(const TCounter start, const TCounter length) {
        ASSERT(start + length <= size());
        return ArrayView(data + start, length);
    }

    INLINE bool operator!() const {
        return data == nullptr;
    }

    INLINE explicit operator bool() const {
        return data != nullptr;
    }

    /// Comparison operator, comparings arrayviews element-by-element.
    bool operator==(const ArrayView& other) const {
        if (actSize != other.actSize) {
            return false;
        }
        for (TCounter i = 0; i < actSize; ++i) {
            if (data[i] != other[i]) {
                return false;
            }
        }
        return true;
    }
};


NAMESPACE_SPH_END
