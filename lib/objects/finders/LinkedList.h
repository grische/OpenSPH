#pragma once

#include "geometry/Box.h"
#include "objects/finders/AbstractFinder.h"
#include "objects/wrappers/Iterators.h"
#include <algorithm>

NAMESPACE_SPH_BEGIN

class LinkedList : public Abstract::Finder {
private:
    VectorOrder sortedIndices;
    VectorOrder rank;


    class LookupMap : public Noncopyable {
    private:
        Array<Size> storage;
        Size dimensionSize;

        INLINE Size map(const Indices& v) const {
            return v[X] * Math::sqr(dimensionSize) + v[Y] * dimensionSize + v[Z];
        }

    public:
        LookupMap() = default;

        LookupMap(const Size n)
            : storage(Math::pow<3>(n))
            , dimensionSize(n) {
            // clear all cells
            storage.fill(0);
        }

        LookupMap& operator=(LookupMap&& other) {
            storage = std::move(other.storage);
            dimensionSize = other.dimensionSize;
            return *this;
        }

        INLINE Size operator()(const Indices& v) const {
            const Size idx = map(v);
            ASSERT(unsigned(idx) < unsigned(storage.size()));
            return storage[idx];
        }

        INLINE Size& operator()(const Indices& v) {
            const Size idx = map(v);
            ASSERT(unsigned(idx) < unsigned(storage.size()));
            return storage[idx];
        }
    };
    LookupMap map;
    Size cellCnt;
    Array<Vector> lowerBounds;
    Array<Vector> upperBounds;
    Array<Size> linkedList;


public:
    LinkedList() = default;

    virtual Size findNeighbours(const Size index,
        const Float radius,
        Array<NeighbourRecord>& neighbours,
        Flags<FinderFlags> flags = EMPTY_FLAGS,
        const Float UNUSED(error) = 0.f) const override {
        neighbours.clear();
        const Float cellCntSqrInv = 1._f / Math::sqr(cellCnt);
        const Box bounds(this->values[index] - Vector(radius), this->values[index] + Vector(radius));
        Indices refRank = rank[index];
        Indices lower(Vector(refRank) * cellCntSqrInv);
        Indices upper = lower;
        for (uint i = 0; i < 3; ++i) {
            while (lower[i] > 0 && bounds.lower()[i] <= lowerBounds[lower[i]][i]) {
                lower[i]--;
            }
        }
        for (uint i = 0; i < 3; ++i) {
            while (
                Size(upper[i]) < upperBounds.size() - 1 && bounds.upper()[i] <= upperBounds[upper[i]][i]) {
                upper[i]++;
            }
        }
        lower = Math::max(lower, Indices(0));
        upper = Math::min(upper, Indices(upperBounds.size() - 1));
        const Size refRankH =
            flags.has(FinderFlags::FIND_ONLY_SMALLER_H) ? rank[index][H] : this->values.size();
        for (int x = lower[X]; x <= upper[X]; ++x) {
            for (int y = lower[Y]; y <= upper[Y]; ++y) {
                for (int z = lower[Z]; z <= upper[Z]; ++z) {
                    const Indices idxs(x, y, z);
                    Size cell = map(idxs);
                    while (cell != 0) {
                        const Float lengthSqr = getSqrLength(this->values[cell] - this->values[index]);
                        if (Size(rank[cell][H]) < refRankH && lengthSqr < Math::sqr(radius)) {
                            neighbours.push(NeighbourRecord{ cell, lengthSqr });
                        }
                        cell = linkedList[cell];
                    }
                }
            }
        }
        return neighbours.size();
    }

protected:
    virtual void rebuildImpl() override {
        for (uint i = 0; i < 3; ++i) {
            sortedIndices.shuffle(i, [this, i](Size idx1, Size idx2) {
                return this->values[idx1][i] < this->values[idx2][i];
            });
        }
        /// extra dimension - sort smoothing length
        sortedIndices.shuffle(
            H, [this](Size idx1, Size idx2) { return this->values[idx1][H] < this->values[idx2][H]; });
        rank = sortedIndices.getInverted();
        map = LookupMap(cellCnt);
        lowerBounds.fill(Vector(INFTY));
        upperBounds.fill(Vector(-INFTY));
        const Float cellCntSqrInv = 1._f / Math::sqr(cellCnt);

        for (Size idx = 0; idx < this->values.size(); ++idx) {
            const Indices multiIdx(Vector(rank[idx]) * cellCntSqrInv);
            Size& cell = map(multiIdx);
            linkedList[idx] = cell;
            cell = idx;
            /// \todo optimize using multiindices
            for (uint i = 0; i < 3; ++i) {
                Float& lb = lowerBounds[multiIdx[i]][i];
                lb = Math::min(lb, this->values[idx][i]);
                Float& ub = upperBounds[multiIdx[i]][i];
                ub = Math::max(ub, this->values[idx][i]);
            }
        }
    }

    virtual void buildImpl(ArrayView<const Vector> values) override {
        sortedIndices = VectorOrder(values.size());
        rank = VectorOrder(values.size());
        linkedList.resize(values.size());
        cellCnt = Math::root<3>(values.size()) + 1;

        lowerBounds.resize(cellCnt);
        upperBounds.resize(cellCnt);
        rebuildImpl();
    }
};

NAMESPACE_SPH_END
