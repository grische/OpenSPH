#pragma once

/// \file OutputIterators.h
/// \brief Helper iterators allowing to save values to containers.
/// \author Pavel Sevecek (sevecek at sirrah.troja.mff.cuni.cz)
/// \date 2016-2021

#include "common/Assert.h"
#include "objects/wrappers/Function.h"
#include <type_traits>

NAMESPACE_SPH_BEGIN

struct OutputIterator {
    using iterator_category = std::output_iterator_tag;
    using value_type = void;
    using difference_type = ptrdiff_t;
    using pointer = void;
    using reference = void;
};

/// \brief Helper output iterator that simply ignores the written values.
class NullInserter : public OutputIterator {
public:
    NullInserter& operator*() {
        return *this;
    }

    NullInserter& operator++() {
        return *this;
    }

    template <typename TValue>
    NullInserter& operator=(TValue&&) {
        return *this;
    }
};


/// \brief Output iterator that inserts the written values to the given container using \ref insert function.
template <typename TContainer>
class Inserter : public OutputIterator {
private:
    TContainer& container;

public:
    explicit Inserter(TContainer& container)
        : container(container) {}

    Inserter& operator*() {
        return *this;
    }

    Inserter& operator++() {
        return *this;
    }

    template <typename TValue>
    Inserter& operator=(TValue&& value) {
        container.insert(std::forward<TValue>(value));
        return *this;
    }
};

template <typename TContainer>
Inserter<TContainer> inserter(TContainer& c) {
    return Inserter<TContainer>(c);
}


/// \brief Output iterator that inserts the written values to the given container using \ref push function.
template <typename TContainer>
class BackInserter : public OutputIterator {
private:
    TContainer* container;

public:
    explicit BackInserter(TContainer& container)
        : container(&container) {}

    BackInserter& operator*() {
        return *this;
    }

    BackInserter& operator++() {
        return *this;
    }

    template <typename TValue,
        typename = std::enable_if_t<!std::is_same<std::decay_t<TValue>, BackInserter>::value>>
    BackInserter& operator=(TValue&& value) {
        container->push(std::forward<TValue>(value));
        return *this;
    }
};

template <typename TContainer>
BackInserter<TContainer> backInserter(TContainer& c) {
    return BackInserter<TContainer>(c);
}

/// \brief Output iterator that calls given functor for all written values.
template<typename TValue>
class FunctorCaller : public OutputIterator {
private:
    Function<void(const TValue&)> func;

public:
    FunctorCaller(Function<void(const TValue&)>&& func)
        : func(std::move(func)) {}

    FunctorCaller& operator*() {
        return *this;
    }

    FunctorCaller& operator++() {
        return *this;
    }

    FunctorCaller& operator=(const TValue& value) {
        func(value);
        return *this;
    }

    FunctorCaller& operator=(TValue&& value) {
        func(std::move(value));
        return *this;
    }
};

NAMESPACE_SPH_END
