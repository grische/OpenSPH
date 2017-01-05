#pragma once

/// Base class defining interface for kNN queries.
/// Pavel Sevecek 2016
/// sevecek at sirrah.troja.mff.cuni.cz

#include "geometry/Vector.h"
#include "objects/containers/Array.h"
#include "objects/finders/Order.h"
#include "objects/wrappers/Flags.h"

NAMESPACE_SPH_BEGIN

struct NeighbourRecord {
    Size index;
    Float distanceSqr;
};

enum class FinderFlags {
    /// Finds only neighbours that have smaller smoothing length h than value given
    FIND_ONLY_SMALLER_H = 1 << 0,

    /// Parallelize the search using all available threads
    PARALLELIZE = 1 << 1,
};

namespace Abstract {
    class Finder : public Polymorphic {
    protected:
        ArrayView<const Vector> values;
        Order rankH;

        virtual void buildImpl(ArrayView<const Vector> values) = 0;

        virtual void rebuildImpl() {}

    public:
        /// Constructs the struct with an array of vectors
        void build(ArrayView<const Vector> values) {
            this->values = values;
            makeRankH();
            buildImpl(values);
        }

        /// Finds all points within given radius from a point.
        /// \param point
        /// \param radius
        /// \param neighbours List of neighbours, as indices to the array
        /// \param error Approximate solution
        /// \return The number of neighbours.


        virtual Size findNeighbours(const Size index,
            const Float radius,
            Array<NeighbourRecord>& neighbours,
            Flags<FinderFlags> flags = EMPTY_FLAGS,
            const Float error = 0._f) const = 0;

        /// Updates the structure when the position change.
        void rebuild() {
            makeRankH();
            rebuildImpl();
        }

    private:
        void makeRankH() {
            Order tmp(this->values.size());
            // sort by smoothing length
            tmp.shuffle([this](const Size i1, const Size i2) { return values[i1][H] < values[i2][H]; });
// invert to get rank in H
/// \todo avoid allocation here
#ifdef DEBUG
            Float lastH = 0._f;
            for (Size i = 0; i < tmp.size(); ++i) {
                ASSERT(this->values[tmp[i]][H] >= lastH);
                lastH = this->values[tmp[i]][H];
            }
#endif
            rankH = tmp.getInverted();
        }
    };
}

NAMESPACE_SPH_END
