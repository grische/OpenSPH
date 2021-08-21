#pragma once

/// \file SymmetricTensor.h
/// \brief Basic algebra for symmetric 2nd order tensors
/// \author Pavel Sevecek (sevecek at sirrah.troja.mff.cuni.cz)
/// \date 2016-2021

#include "math/AffineMatrix.h"
#include "objects/containers/StaticArray.h"

NAMESPACE_SPH_BEGIN

struct Svd;

/// \brief Symmetric tensor of 2nd order.
///
/// Internally, data are stored in two vectors, representing the diagonal and the off-diagonal components.
class SymmetricTensor {
private:
    Vector diag; // diagonal part
    Vector off;  // over/below diagonal

public:
    SymmetricTensor() = default;

    INLINE SymmetricTensor(const SymmetricTensor& other)
        : diag(other.diag)
        , off(other.off) {}

    /// \brief Construct tensor given its diagonal vector and a vector of off-diagonal elements (sorted
    /// top-bottom and left-right).
    INLINE SymmetricTensor(const Vector& diag, const Vector& off)
        : diag(diag)
        , off(off) {}

    /// \brief Initialize all components of the tensor to given value.
    INLINE explicit SymmetricTensor(const Float value)
        : diag(value)
        , off(value) {}

    /// \brief Construct tensor given three vectors as rows.
    ///
    /// Matrix represented by the vectors MUST be symmetric, checked by assert.
    INLINE SymmetricTensor(const Vector& v0, const Vector& v1, const Vector& v2) {
        SPH_ASSERT(v0[1] == v1[0], v0[1], v1[0]);
        SPH_ASSERT(v0[2] == v2[0], v0[2], v2[0]);
        SPH_ASSERT(v1[2] == v2[1], v1[2], v2[1]);
        diag = Vector(v0[0], v1[1], v2[2]);
        off = Vector(v0[1], v0[2], v1[2]);
    }

    INLINE SymmetricTensor& operator=(const SymmetricTensor& other) {
        diag = other.diag;
        off = other.off;
        return *this;
    }

    /// Returns a row of the matrix.
    INLINE Vector row(const Size idx) const {
        SPH_ASSERT(idx < 3);
        switch (idx) {
        case 0:
            return Vector(diag[0], off[0], off[1]);
        case 1:
            return Vector(off[0], diag[1], off[2]);
        case 2:
            return Vector(off[1], off[2], diag[2]);
        default:
            STOP;
        }
    }

    /// Returns a given element of the matrix.
    INLINE Float& operator()(const Size rowIdx, const Size colIdx) {
        if (rowIdx == colIdx) {
            return diag[rowIdx];
        } else {
            return off[rowIdx + colIdx - 1];
        }
    }

    /// Returns a given element of the matrix, const version.
    INLINE Float operator()(const Size rowIdx, const Size colIdx) const {
        if (rowIdx == colIdx) {
            return diag[rowIdx];
        } else {
            return off[rowIdx + colIdx - 1];
        }
    }

    /// Returns the diagonal part of the tensor
    INLINE const Vector& diagonal() const {
        return diag;
    }

    /// Returns the off-diagonal elements of the tensor
    INLINE const Vector& offDiagonal() const {
        return off;
    }

    /// Applies the tensor on given vector
    INLINE Vector operator*(const Vector& v) const {
        /// \todo optimize using _mm_shuffle_ps
        return Vector(diag[0] * v[0] + off[0] * v[1] + off[1] * v[2],
            off[0] * v[0] + diag[1] * v[1] + off[2] * v[2],
            off[1] * v[0] + off[2] * v[1] + diag[2] * v[2]);
    }

    /// Multiplies a tensor by a scalar
    INLINE friend SymmetricTensor operator*(const SymmetricTensor& t, const Float v) {
        return SymmetricTensor(t.diag * v, t.off * v);
    }

    INLINE friend SymmetricTensor operator*(const Float v, const SymmetricTensor& t) {
        return SymmetricTensor(t.diag * v, t.off * v);
    }

    /// Multiplies a tensor by another tensor, element-wise. Not a matrix multiplication!
    INLINE friend SymmetricTensor operator*(const SymmetricTensor& t1, const SymmetricTensor& t2) {
        return SymmetricTensor(t1.diag * t2.diag, t1.off * t2.off);
    }

    /// Divides a tensor by a scalar
    INLINE friend SymmetricTensor operator/(const SymmetricTensor& t, const Float v) {
        return SymmetricTensor(t.diag / v, t.off / v);
    }

    /// Divides a tensor by another tensor, element-wise.
    INLINE friend SymmetricTensor operator/(const SymmetricTensor& t1, const SymmetricTensor& t2) {
        return SymmetricTensor(t1.diag / t2.diag, t1.off / t2.off);
    }

    /// Sums up two tensors
    INLINE friend SymmetricTensor operator+(const SymmetricTensor& t1, const SymmetricTensor& t2) {
        return SymmetricTensor(t1.diag + t2.diag, t1.off + t2.off);
    }

    INLINE friend SymmetricTensor operator-(const SymmetricTensor& t1, const SymmetricTensor& t2) {
        return SymmetricTensor(t1.diag - t2.diag, t1.off - t2.off);
    }

    INLINE SymmetricTensor& operator+=(const SymmetricTensor& other) {
        diag += other.diag;
        off += other.off;
        return *this;
    }

    INLINE SymmetricTensor& operator-=(const SymmetricTensor& other) {
        diag -= other.diag;
        off -= other.off;
        return *this;
    }

    INLINE SymmetricTensor& operator*=(const Float value) {
        diag *= value;
        off *= value;
        return *this;
    }

    INLINE SymmetricTensor& operator/=(const Float value) {
        diag /= value;
        off /= value;
        return *this;
    }

    INLINE SymmetricTensor operator-() const {
        return SymmetricTensor(-diag, -off);
    }

    /* INLINE explicit operator Tensor() const {
         return Tensor((*this)[0], (*this)[1], (*this)[2]);
     }*/

    INLINE bool operator==(const SymmetricTensor& other) const {
        return diag == other.diag && off == other.off;
    }

    INLINE bool operator!=(const SymmetricTensor& other) const {
        return diag != other.diag || off != other.off;
    }

    /// Returns an identity tensor.
    INLINE static SymmetricTensor identity() {
        return SymmetricTensor(Vector(1._f, 1._f, 1._f), Vector(0._f, 0._f, 0._f));
    }

    /// Returns a tensor with all zeros.
    INLINE static SymmetricTensor null() {
        return SymmetricTensor(Vector(0._f), Vector(0._f));
    }

    /// Returns the determinant of the tensor
    INLINE Float determinant() const {
        return diag[0] * diag[1] * diag[2] + 2 * off[0] * off[1] * off[2] -
               (dot(sqr(off), Vector(diag[2], diag[1], diag[0])));
    }

    /// Return the trace of the tensor
    INLINE Float trace() const {
        return dot(diag, Vector(1._f));
    }

    /// Returns n-th invariant of the tensor (1<=n<=3)
    template <int n>
    INLINE Float invariant() const {
        switch (n) {
        case 1:
            return trace();
        case 2:
            /// \todo optimize with shuffle
            return getSqrLength(off) - (diag[1] * diag[2] + diag[2] * diag[0] + diag[0] * diag[1]);
        case 3:
            return determinant();
        default:
            STOP;
        }
    }

    INLINE SymmetricTensor inverse() const {
        const Float det = determinant();
        SPH_ASSERT(det != 0._f);
        Vector invDiag, invOff;
        /// \todo optimize using SSE
        invDiag[0] = diag[1] * diag[2] - sqr(off[2]);
        invDiag[1] = diag[2] * diag[0] - sqr(off[1]);
        invDiag[2] = diag[0] * diag[1] - sqr(off[0]);
        invOff[0] = off[1] * off[2] - diag[2] * off[0];
        invOff[1] = off[2] * off[0] - diag[1] * off[1];
        invOff[2] = off[0] * off[1] - diag[0] * off[2];
        return SymmetricTensor(invDiag / det, invOff / det);
    }

    /// Moore-Penrose pseudo-inversion of matrix
    SymmetricTensor pseudoInverse(const Float eps) const;

    template <typename TStream>
    friend TStream& operator<<(TStream& stream, const SymmetricTensor& t) {
        stream << t.diagonal() << t.offDiagonal();
        return stream;
    }
};

/// Transforms given symmetric tensor by matrix.
/// \todo optimize
INLINE SymmetricTensor transform(const SymmetricTensor& t, const AffineMatrix& transform) {
    const AffineMatrix m(t.row(0), t.row(1), t.row(2));
    AffineMatrix transformed = transform * m * transform.inverse();
    return SymmetricTensor(Vector(transformed(0, 0), transformed(1, 1), transformed(2, 2)),
        0.5_f * Vector(transformed(0, 1) + transformed(1, 0),
                    transformed(0, 2) + transformed(2, 0),
                    transformed(1, 2) + transformed(2, 1)));
}

template <typename T1, typename T2>
T1 convert(const T2& matrix);

template <>
INLINE SymmetricTensor convert(const AffineMatrix& matrix) {
    SPH_ASSERT(almostEqual(matrix(0, 1), matrix(1, 0), 1.e-6_f) &&
               almostEqual(matrix(0, 2), matrix(2, 0), 1.e-6_f) &&
               almostEqual(matrix(1, 2), matrix(2, 1), 1.e-6_f));
    return SymmetricTensor(
        Vector(matrix(0, 0), matrix(1, 1), matrix(2, 2)), Vector(matrix(0, 1), matrix(0, 2), matrix(1, 2)));
}

template <>
INLINE AffineMatrix convert(const SymmetricTensor& t) {
    // make sure there is no 'accidental' translation in the matrix
    SPH_ASSERT(t.row(0)[H] == 0._f);
    SPH_ASSERT(t.row(1)[H] == 0._f);
    SPH_ASSERT(t.row(2)[H] == 0._f);
    return AffineMatrix(t.row(0), t.row(1), t.row(2));
}


/// Tensor utils


/// Checks if two tensors are equal to some given accuracy.
INLINE bool almostEqual(const SymmetricTensor& t1, const SymmetricTensor& t2, const Float eps = EPS) {
    return almostEqual(t1.diagonal(), t2.diagonal(), eps) &&
           almostEqual(t1.offDiagonal(), t2.offDiagonal(), eps);
}

/// Arbitrary norm of the tensor.
///
/// \note This norm is NOT an invariant.
/// \todo Use some well-defined norm instead? (spectral norm, L1 or L2 norm, ...)
template <>
INLINE Float norm(const SymmetricTensor& t) {
    const Vector v = max(t.diagonal(), t.offDiagonal());
    SPH_ASSERT(isReal(v));
    return norm(v);
}

/// Arbitrary squared norm of the tensor
template <>
INLINE Float normSqr(const SymmetricTensor& t) {
    const Vector v = max(t.diagonal(), t.offDiagonal());
    return normSqr(v);
}

/// Returns the tensor of absolute values
template <>
INLINE SymmetricTensor abs(const SymmetricTensor& t) {
    return SymmetricTensor(abs(t.diagonal()), abs(t.offDiagonal()));
}

/// Returns the minimal element of the tensor.
template <>
INLINE Float minElement(const SymmetricTensor& t) {
    return min(minElement(t.diagonal()), minElement(t.offDiagonal()));
}

/// Returns the maximal element of the tensor.
template <>
INLINE Float maxElement(const SymmetricTensor& t) {
    return max(maxElement(t.diagonal()), maxElement(t.offDiagonal()));
}

/// Component-wise minimum of two tensors.
template <>
INLINE SymmetricTensor min(const SymmetricTensor& t1, const SymmetricTensor& t2) {
    return SymmetricTensor(min(t1.diagonal(), t2.diagonal()), min(t1.offDiagonal(), t2.offDiagonal()));
}

/// Component-wise maximum of two tensors.
template <>
INLINE SymmetricTensor max(const SymmetricTensor& t1, const SymmetricTensor& t2) {
    return SymmetricTensor(max(t1.diagonal(), t2.diagonal()), max(t1.offDiagonal(), t2.offDiagonal()));
}

/// Clamping all components by range.
template <>
INLINE SymmetricTensor clamp(const SymmetricTensor& t, const Interval& range) {
    return SymmetricTensor(clamp(t.diagonal(), range), clamp(t.offDiagonal(), range));
}

template <>
INLINE bool isReal(const SymmetricTensor& t) {
    return isReal(t.diagonal()) && isReal(t.offDiagonal());
}

template <>
INLINE auto less(const SymmetricTensor& t1, const SymmetricTensor& t2) {
    return SymmetricTensor(less(t1.diagonal(), t2.diagonal()), less(t1.offDiagonal(), t2.offDiagonal()));
}

template <>
INLINE StaticArray<Float, 6> getComponents(const SymmetricTensor& t) {
    return { t(0, 0), t(1, 1), t(2, 2), t(0, 1), t(0, 2), t(1, 2) };
}

/// Double-dot product t1 : t2 = sum_ij t1_ij t2_ij
INLINE Float ddot(const SymmetricTensor& t1, const SymmetricTensor& t2) {
    return dot(t1.diagonal(), t2.diagonal()) + 2._f * dot(t1.offDiagonal(), t2.offDiagonal());
}

/// \brief SYMMETRIZED outer product of two vectors.
///
/// Note that simple outer product is not necessarily symmetric matrix.
INLINE SymmetricTensor symmetricOuter(const Vector& v1, const Vector& v2) {
    /// \todo optimize
    return SymmetricTensor(v1 * v2,
        0.5_f * Vector(v1[0] * v2[1] + v1[1] * v2[0],
                    v1[0] * v2[2] + v1[2] * v2[0],
                    v1[1] * v2[2] + v1[2] * v2[1]));
}

/// Cosine applied to all components of the vector.
INLINE Vector vectorCos(const Vector& v) {
    /// \todo return _mm_cos_ps(v.sse());
    return Vector(cos(v[X]), cos(v[Y]), cos(v[Z]));
}

/// Returns three eigenvalue of symmetric matrix.
INLINE StaticArray<Float, 3> findEigenvalues(const SymmetricTensor& t) {
    const Float n = norm(t);
    if (n < 1.e-12_f) {
        return { 0._f, 0._f, 0._f };
    }
    const Float p = -t.invariant<1>() / n;
    const Float q = -t.invariant<2>() / sqr(n);
    const Float r = -t.invariant<3>() / pow<3>(n);

    const Float a = q - p * p / 3._f;
    const Float b = (2._f * pow<3>(p) - 9._f * p * q + 27._f * r) / 27._f;
    const Float aCub = pow<3>(a) / 27._f;
    if (0.25_f * b * b + aCub >= 0._f) {
        return { 0._f, 0._f, 0._f };
    }
    SPH_ASSERT(a < 0._f);
    const Float t1 = 2._f * sqrt(-a / 3._f);
    const Float phi = acos(-0.5_f * b / sqrt(-aCub));
    const Vector v(phi / 3._f, (phi + 2 * PI) / 3._f, (phi + 4 * PI) / 3._f);
    const Vector sig = t1 * vectorCos(v) - Vector(p / 3._f);
    return { sig[0] * n, sig[1] * n, sig[2] * n };
}

struct Eigen {
    /// Matrix of eigenvectors, stored as rows
    AffineMatrix vectors;

    /// Eigenvalues
    Vector values;
};

/// Computes eigenvectors and corresponding eigenvalues of symmetric matrix.
Eigen eigenDecomposition(const SymmetricTensor& t);

struct Svd {
    AffineMatrix U;
    Vector S;
    AffineMatrix V;
};

/// Computes the singular value decomposition of symmetric matrix.
Svd singularValueDecomposition(const SymmetricTensor& t);

NAMESPACE_SPH_END
