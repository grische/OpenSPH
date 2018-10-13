#pragma once

/// \file Domain.h
/// \brief Object defining computational domain.
/// \author Pavel Sevecek (sevecek at sirrah.troja.mff.cuni.cz)
/// \date 2016-2018

#include "math/AffineMatrix.h"
#include "objects/geometry/Box.h"
#include "objects/wrappers/Optional.h"

NAMESPACE_SPH_BEGIN

/// \todo rename
enum class SubsetType {
    INSIDE,  ///< Marks all vectors inside of the domain
    OUTSIDE, ///< Marks all vectors outside of the domain
};


struct Ghost {
    Vector position; ///< Position of the ghost
    Size index;      ///< Index into the original array of vectors
};


/// \brief Base class for computational domains.
class IDomain : public Polymorphic {
protected:
    Vector center;

public:
    /// \brief Constructs the domain given its center point.
    IDomain(const Vector& center)
        : center(center) {}

    /// \brief Returns the center of the domain.
    virtual Vector getCenter() const {
        return this->center;
    }

    /// \brief Returns the bounding box of the domain.
    virtual Box getBoundingBox() const = 0;

    /// \brief Returns the total volume of the domain.
    ///
    /// This should be identical to computing an integral of \ref isInside function, although faster and more
    /// precise.
    virtual Float getVolume() const = 0;

    /// \brief Checks if the given point lies inside the domain.
    ///
    /// Points lying exactly on the boundary of the domain are assumed to be inside.
    virtual bool contains(const Vector& v) const = 0;

    /// \brief Returns an array of indices, marking vectors with given property by their index.
    ///
    /// \param vs Input array of points.
    /// \param output Output array, is not cleared by the method, previously stored values are kept
    ///               unchanged. Indices of vectors belonging in the subset are pushed into the array.
    /// \param type Type of the subset, see \ref SubsetType.
    virtual void getSubset(ArrayView<const Vector> vs, Array<Size>& output, const SubsetType type) const = 0;

    /// \brief Returns distances of particles lying close to the boundary.
    ///
    /// The distances are signed, negative number means the particle is lying outside of the domain. Distances
    /// can be computed with small error to simplify implementation.
    /// \param vs Input array of points.
    /// \param distances Output array, will be resized to the size of particle array and cleared.
    virtual void getDistanceToBoundary(ArrayView<const Vector> vs, Array<Float>& distances) const = 0;

    /// \brief Projects vectors outside of the domain onto its boundary.
    ///
    /// Vectors inside the domain are untouched. Function does not affect 4th component of vectors.
    /// \param vs Array of vectors we want to project
    /// \param indices Optional array of indices. If passed, only selected vectors will be projected. All
    ///        vectors are projected by default.
    virtual void project(ArrayView<Vector> vs, Optional<ArrayView<Size>> indices = NOTHING) const = 0;

    /// \brief Duplicates vectors located close to the boundary, placing the symmetrically to the other side.
    ///
    /// Distance of the copy (ghost) to the boundary shall be the same as the source vector. One vector
    /// can create multiple ghosts.
    /// \param vs Array containing vectors creating ghosts.
    /// \param ghosts Output parameter containing created ghosts, stored as pairs (position of ghost and
    ///        index of source vector). Array must be cleared by the function!
    /// \param radius Dimensionless distance to the boundary necessary for creating a ghost. A ghost is
    ///        created for vector v if it is closer than radius * v[H]. Vector must be inside, outside
    ///        vectors are ignored.
    /// \param eps Minimal dimensionless distance of ghost from the source vector. When vector is too
    ///        close to the boundary, the ghost would be too close or even on top of the source vector;
    ///        implementation must place the ghost so that it is outside of the domain and at least
    ///        eps * v[H] from the vector. Must be strictly lower than radius, checked by assert.
    virtual void addGhosts(ArrayView<const Vector> vs,
        Array<Ghost>& ghosts,
        const Float radius = 2._f,
        const Float eps = 0.05_f) const = 0;

    /// \todo function for transforming block [0, 1]^d into the domain?
};


/// \brief Spherical domain, defined by the center of sphere and its radius.
class SphericalDomain : public IDomain {
private:
    Float radius;

public:
    SphericalDomain(const Vector& center, const Float& radius);

    virtual Float getVolume() const override;

    virtual Box getBoundingBox() const override;

    virtual bool contains(const Vector& v) const override;

    virtual void getSubset(ArrayView<const Vector> vs,
        Array<Size>& output,
        const SubsetType type) const override;

    virtual void getDistanceToBoundary(ArrayView<const Vector> vs, Array<Float>& distances) const override;

    virtual void project(ArrayView<Vector> vs, Optional<ArrayView<Size>> indices = NOTHING) const override;

    virtual void addGhosts(ArrayView<const Vector> vs,
        Array<Ghost>& ghosts,
        const Float eta,
        const Float eps) const override;

private:
    INLINE bool isInsideImpl(const Vector& v) const {
        return getSqrLength(v - this->center) <= sqr(radius);
    }
};

/// \brief Axis aligned ellipsoidal domain, defined by the center of sphere and lengths of three axes.
class EllipsoidalDomain : public IDomain {
private:
    /// Lengths of axes
    Vector radii;

    /// Effective radius (radius of a sphere with same volume)
    Float effectiveRadius;

public:
    EllipsoidalDomain(const Vector& center, const Vector& axes);

    virtual Float getVolume() const override;

    virtual Box getBoundingBox() const override;

    virtual bool contains(const Vector& v) const override;

    virtual void getSubset(ArrayView<const Vector> vs,
        Array<Size>& output,
        const SubsetType type) const override;

    virtual void getDistanceToBoundary(ArrayView<const Vector> vs, Array<Float>& distances) const override;

    virtual void project(ArrayView<Vector> vs, Optional<ArrayView<Size>> indices = NOTHING) const override;

    virtual void addGhosts(ArrayView<const Vector> vs,
        Array<Ghost>& ghosts,
        const Float eta,
        const Float eps) const override;

private:
    INLINE bool isInsideImpl(const Vector& v) const {
        return getSqrLength((v - this->center) / radii) <= 1.f;
    }
};


/// \brief Block aligned with coordinate axes, defined by its center and length of each side.
/// \todo create extra ghosts in the corners?
class BlockDomain : public IDomain {
private:
    Box box;

public:
    BlockDomain(const Vector& center, const Vector& edges);

    virtual Float getVolume() const override;

    virtual Box getBoundingBox() const override;

    virtual bool contains(const Vector& v) const override;

    virtual void getSubset(ArrayView<const Vector> vs,
        Array<Size>& output,
        const SubsetType type) const override;

    virtual void getDistanceToBoundary(ArrayView<const Vector> vs, Array<Float>& distances) const override;

    virtual void project(ArrayView<Vector> vs, Optional<ArrayView<Size>> indices = NOTHING) const override;

    virtual void addGhosts(ArrayView<const Vector> vs,
        Array<Ghost>& ghosts,
        const Float eta,
        const Float eps) const override;
};


/// \brief Cylinder aligned with z-axis, optionally including bases (can be either open or close cylinder).
class CylindricalDomain : public IDomain {
private:
    Float radius;
    Float height;
    bool includeBases;

public:
    CylindricalDomain(const Vector& center, const Float radius, const Float height, const bool includeBases);

    virtual Float getVolume() const override;

    virtual Box getBoundingBox() const override;

    virtual bool contains(const Vector& v) const override;

    virtual void getSubset(ArrayView<const Vector> vs,
        Array<Size>& output,
        const SubsetType type) const override;

    virtual void getDistanceToBoundary(ArrayView<const Vector> vs, Array<Float>& distances) const override;

    virtual void project(ArrayView<Vector> vs, Optional<ArrayView<Size>> indices = NOTHING) const override;

    virtual void addGhosts(ArrayView<const Vector> vs,
        Array<Ghost>& ghosts,
        const Float eta,
        const Float eps) const override;

private:
    INLINE bool isInsideImpl(const Vector& v) const {
        return getSqrLength(Vector(v[X], v[Y], this->center[Z]) - center) <= sqr(radius) &&
               sqr(v[Z] - this->center[Z]) <= sqr(0.5_f * height);
    }
};


/// \brief Similar to cylindrical domain, but bases are hexagons instead of circles.
///
/// Hexagons are oriented so that two sides are parallel with x-axis.
/// \todo could be easily generalized to any polygon, currently not needed though
class HexagonalDomain : public IDomain {
private:
    Float outerRadius; // bounding radius of the base
    Float innerRadius;
    Float height;
    bool includeBases;

public:
    HexagonalDomain(const Vector& center, const Float radius, const Float height, const bool includeBases);

    virtual Float getVolume() const override;

    virtual Box getBoundingBox() const override;

    virtual bool contains(const Vector& v) const override;

    virtual void getSubset(ArrayView<const Vector> vs,
        Array<Size>& output,
        const SubsetType type) const override;

    virtual void getDistanceToBoundary(ArrayView<const Vector>, Array<Float>&) const override;

    virtual void project(ArrayView<Vector> vs, Optional<ArrayView<Size>> indices = NOTHING) const override;

    virtual void addGhosts(ArrayView<const Vector> vs,
        Array<Ghost>& ghosts,
        const Float eta,
        const Float eps) const override;

private:
    INLINE bool isInsideImpl(const Vector& v) const {
        if (sqr(v[Z] - this->center[Z]) > sqr(0.5_f * height)) {
            return false;
        }
        const Float sqrLength = getSqrLength(v);
        if (sqrLength > sqr(outerRadius)) {
            return false;
        }
        if (sqrLength <= sqr(innerRadius)) {
            return true;
        }
        const Float phi = atan2(v[Y], v[X]);
        return getSqrLength(Vector(v[X], v[Y], 0._f)) <= sqr(hexagon(phi));
    }

    /// Polar plot of hexagon
    INLINE Float hexagon(const Float phi) const {
        return 0.5_f * SQRT_3 * 1._f / sin(phi - PI / 3._f * (floor(phi / (PI / 3._f)) - 1._f));
    }
};

/// \brief Transform another domain by given transformation matrix
///
/// \todo TESTS
template <typename TDomain>
class TransformedDomain : public IDomain {
private:
    TDomain domain;
    AffineMatrix tm, tmInv;

public:
    template <typename... TArgs>
    TransformedDomain(const AffineMatrix& matrix, const Vector& center, TArgs&&... args)
        : IDomain(matrix.inverse() * center)
        , domain(matrix.inverse() * center, std::forward<TArgs>(args)...) {
        tm = matrix;
        tmInv = matrix.inverse();
    }

    virtual Float getVolume() const override {
        return domain.getVolume() * tm.determinant();
    }

    virtual Box getBoundingBox() const override {
        const Box box = domain.getBoundingBox();
        // transform all 8 points
        Box transformedBox;
        for (Size i = 0; i <= 1; ++i) {
            for (Size j = 0; j <= 1; ++j) {
                for (Size k = 0; k <= 1; ++k) {
                    const Vector p =
                        box.lower() * Vector(i, j, k) + box.upper() * Vector(1 - i, 1 - j, 1 - k);
                    transformedBox.extend(tm * p);
                }
            }
        }
        return transformedBox;
    }

    virtual bool contains(const Vector& v) const override {
        return domain.contains(tmInv * v);
    }

    virtual void getSubset(ArrayView<const Vector> vs,
        Array<Size>& output,
        const SubsetType type) const override {
        domain.getSubset(this->untransform(vs), output, type);
    }

    virtual void getDistanceToBoundary(ArrayView<const Vector> vs, Array<Float>& distances) const override {
        return domain.getDistanceToBoundary(this->untransform(vs), distances);
    }

    virtual void project(ArrayView<Vector> vs, Optional<ArrayView<Size>> indices = NOTHING) const override {
        return domain.project(this->untransform(vs), indices);
    }

    virtual void addGhosts(ArrayView<const Vector> vs,
        Array<Ghost>& ghosts,
        const Float eta,
        const Float eps) const override {
        domain.addGhosts(this->untransform(vs), ghosts, eta, eps);
        for (Ghost& g : ghosts) {
            g.position = tm * g.position;
        }
    }

private:
    Array<Vector> untransform(ArrayView<const Vector> vs) const {
        Array<Vector> untransformed(vs.size());
        for (Size i = 0; i < vs.size(); ++i) {
            untransformed[i] = tmInv * vs[i];
        }
        return untransformed;
    }
};

NAMESPACE_SPH_END
