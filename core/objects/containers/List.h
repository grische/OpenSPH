#pragma once

/// \file List.h
/// \brief Doubly-linked list
/// \author Pavel Sevecek (sevecek at sirrah.troja.mff.cuni.cz)
/// \date 2016-2021

#include "common/Traits.h"
#include "objects/containers/BasicAllocators.h"
#include "objects/wrappers/RawPtr.h"

NAMESPACE_SPH_BEGIN

template <typename T, typename TAllocator = Mallocator>
class List;

template <typename T>
struct ListNode {
    using StorageType = typename WrapReferenceType<T>::Type;

    /// Stored value
    StorageType value;

    /// Next element in the list, can be nullptr.
    RawPtr<ListNode> next;

    /// Previous element in the list, can be nullptr.
    RawPtr<ListNode> prev;

    template <typename U>
    ListNode(U&& value, RawPtr<ListNode> prev, RawPtr<ListNode> next)
        : value(std::forward<U>(value))
        , next(next)
        , prev(prev) {
        // update pointers of neighboring nodes
        if (prev) {
            prev->next = this;
        }
        if (next) {
            next->prev = this;
        }
    }

    /// Removes this element from the list, connecting the previous and the next element.
    /// This element itself must be deleted by the owner afterwards.
    void detach() {
        if (prev) {
            prev->next = next;
        }
        if (next) {
            next->prev = prev;
        }
    }
};

template <typename T>
class ListIterator {
    template <typename, typename>
    friend class List;

private:
    RawPtr<ListNode<T>> node;

public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    explicit ListIterator(RawPtr<ListNode<T>> node)
        : node(node) {}

    INLINE ListIterator& operator++() {
        if (node) {
            node = node->next;
        }
        return *this;
    }

    INLINE ListIterator operator++(int) {
        ListIterator next = *this;
        ++next;
        return next;
    }

    INLINE ListIterator& operator--() {
        if (node) {
            node = node->prev;
        }
        return *this;
    }

    INLINE ListIterator operator--(int) {
        ListIterator prev = *this;
        --prev;
        return prev;
    }

    INLINE T& operator*() const {
        SPH_ASSERT(node);
        return node->value;
    }

    INLINE RawPtr<T> operator->() const {
        SPH_ASSERT(node);
        return &node->value;
    }

    INLINE explicit operator bool() const {
        return bool(node);
    }

    INLINE bool operator!() const {
        return !node;
    }

    bool operator==(const ListIterator& other) const {
        return node == other.node;
    }

    bool operator!=(const ListIterator& other) const {
        return node != other.node;
    }
};

/// \brief Doubly-linked list
///
/// Random access is not implemented, as it would be highly ineffective anyway.
template <typename T, typename TAllocator>
class List : private TAllocator, public Noncopyable {
private:
    using StorageType = typename WrapReferenceType<T>::Type;

    RawPtr<ListNode<T>> first;
    RawPtr<ListNode<T>> last;

public:
    /// Constructs the list with no elements
    List()
        : first(nullptr)
        , last(nullptr) {}

    /// Constructs the list given initializer_list of elements
    List(std::initializer_list<StorageType> list) {
        for (const StorageType& value : list) {
            this->pushBack(value);
        }
    }

    /// Move constructor.
    List(List&& other)
        : first(other.first)
        , last(other.last) {
        other.first = other.last = nullptr;
    }

    ~List() {
        this->clear();
    }

    /// Move operator.
    List& operator=(List&& other) {
        std::swap(first, other.first);
        std::swap(last, other.last);
        return *this;
    }

    /// \brief Returns true if there are no elements in the list
    INLINE bool empty() const {
        return first == nullptr;
    }

    /// \brief Returns the number of elements in the list.
    ///
    /// Note that this is an O(N) operation, the number of elements is not cached.
    INLINE Size size() const {
        Size counter = 0;
        for (RawPtr<ListNode<T>> ptr = first; ptr != nullptr; ptr = ptr->next) {
            ++counter;
        }
        return counter;
    }

    /// \brief Adds a new element to the back of the list.
    ///
    /// Element is copied or moved, based on the type of the value.
    template <typename U>
    void pushBack(U&& value) {
        RawPtr<ListNode<T>> node = newNode(std::forward<U>(value), last, nullptr);
        last = node;
        if (first == nullptr) {
            // this is the first element, update also first ptr.
            first = node;
        }
    }

    /// \brief Adds a new element to the beginning of the list.
    ///
    /// Element is copied or moved, based on the type of the value.
    template <typename U>
    void pushFront(U&& value) {
        RawPtr<ListNode<T>> node = newNode(std::forward<U>(value), nullptr, first);
        first = node;
        if (last == nullptr) {
            // this is the first element, update also last ptr.
            last = node;
        }
    }

    /// Creates a new element and inserts it after the element given by the iterator.
    /// \param iter Iterator to an element of the list. Must not be nullptr.
    template <typename U>
    void insert(const ListIterator<T> iter, U&& value) {
        SPH_ASSERT(iter);
        RawPtr<ListNode<T>> node = newNode(std::forward<U>(value), iter.node, iter.node->next);
        if (iter.node == last) {
            // we added to the back of the list, update the pointer
            last = node;
        }
    }

    /// \brief Removes an element given by the iterator.
    ///
    /// This does not invalidate iterators or pointers to element, except for the iterator to the element
    /// being erased from the list.
    /// \param iter Iterator to an element being erased. Must not be nullptr.
    /// \return Iterator pointing to the next element, or nullptr if the removed element was the last one.
    ListIterator<T> erase(const ListIterator<T> iter) {
        SPH_ASSERT(iter);
        ListIterator<T> next = iter;
        ++next;
        RawPtr<ListNode<T>> node = iter.node;
        node->detach();
        if (node == first) {
            // erasing first element, adjust pointer
            first = first->next;
        }
        if (node == last) {
            // erasing last element, adjust pointer
            last = last->prev;
        }
        if (node) {
            deleteNode(node.get());
        }
        return next;
    }

    /// \brief Removes all elements from the list
    void clear() {
        for (RawPtr<ListNode<T>> ptr = first; ptr != nullptr;) {
            RawPtr<ListNode<T>> next = ptr->next;
            if (ptr) {
                deleteNode(ptr.get());
            }
            ptr = next;
        }
        first = last = nullptr;
    }

    /// Returns the reference to the first element in the list.
    INLINE T& front() {
        SPH_ASSERT(!this->empty());
        return first->value;
    }

    /// Returns the reference to the first element in the list.
    INLINE const T& front() const {
        SPH_ASSERT(!this->empty());
        return first->value;
    }

    /// Returns the reference to the last element in the list.
    INLINE T& back() {
        SPH_ASSERT(!this->empty());
        return last->value;
    }

    /// Returns the reference to the last element in the list.
    INLINE const T& back() const {
        SPH_ASSERT(!this->empty());
        return last->value;
    }

    /// Creates a copy of the list.
    List clone() const {
        List copy;
        for (auto& value : *this) {
            copy.pushBack(value);
        }
        return copy;
    }

    /// Returns a bidirectional iterator pointing to the first element of the list.
    ListIterator<T> begin() {
        return ListIterator<T>(first);
    }

    /// Returns a bidirectional iterator pointing to the first element of the list.
    ListIterator<const T> begin() const {
        // same types, only differ by const-ness
        /// \todo is this ok? better solution?
        return ListIterator<const T>(reinterpret_cast<ListNode<const T>*>(first.get()));
    }

    /// Returns a bidirectional iterator pointing to the one-past-last element of the list.
    ListIterator<T> end() {
        return ListIterator<T>(nullptr);
    }

    /// Returns a bidirectional iterator pointing to the one-past-last element of the list.
    ListIterator<const T> end() const {
        return ListIterator<const T>(nullptr);
    }

    /// Returns the interface to the allocator.
    const TAllocator& allocator() const {
        return *this;
    }

    /// Returns the interface to the allocator.
    TAllocator& allocator() {
        return *this;
    }

    /// Prints content of the list to stream. Stored values must have overloaded << operator.
    template <typename TStream>
    friend TStream& operator<<(TStream& stream, const List& array) {
        for (const T& t : array) {
            stream << t << std::endl;
        }
        return stream;
    }

private:
    template <typename... TArgs>
    RawPtr<ListNode<T>> newNode(TArgs&&... args) {
        MemoryBlock block = TAllocator::allocate(sizeof(ListNode<T>), alignof(ListNode<T>));
        SPH_ASSERT(block.ptr);
        return new (block.ptr) ListNode<T>(std::forward<TArgs>(args)...);
    }

    void deleteNode(ListNode<T>* node) {
        node->~ListNode<T>();
        MemoryBlock block(node, sizeof(ListNode<T>));
        TAllocator::deallocate(block);
    }
};

NAMESPACE_SPH_END
