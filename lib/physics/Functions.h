#pragma once

/// \file Function.h
/// \brief Helper functions used by the code
/// \author Pavel Sevecek
/// \date 2016-2018

#include "math/rng/Rng.h"
#include "objects/geometry/SymmetricTensor.h"
#include "objects/wrappers/Flags.h"
#include "physics/Constants.h"

NAMESPACE_SPH_BEGIN

/// Contains analytic solutions of equations
namespace Analytic {

/// Properties of a homogeneous sphere in rest (no temporal derivatives)
class StaticSphere {
private:
    /// Radius
    Float r0;

    /// Density
    Float rho;

public:
    StaticSphere(const Float r0, const Float rho)
        : r0(r0)
        , rho(rho) {}

    /// Return the pressure at given radius r of a sphere self-compressed by gravity.
    INLINE Float getPressure(const Float r) const {
        if (r > r0) {
            return 0._f;
        }
        return 2._f / 3._f * PI * Constants::gravity * sqr(rho) * (sqr(r0) - sqr(r));
    }

    /// Returns the gravitational acceleration at given radius r. The acceleration increases linearily up
    /// to r0 and then decreases with r^2.
    INLINE Vector getAcceleration(const Vector& r) const {
        const Float l = getLength(r);
        const Float l0 = min(r0, l);
        return -Constants::gravity * rho * sphereVolume(l0) * r / pow<3>(l);
    }
};

} // namespace Analytic

/// Physics of rigid body
namespace Rigid {

/// \brief Computes the inertia tensor of a homogeneous sphere.
INLINE SymmetricTensor sphereInertia(const Float m, const Float r) {
    return SymmetricTensor::identity() * (0.4_f * m * sqr(r));
}

/// \brief Computes the inertia tensor with respect to given point
///
/// \param I Inertia tensor with respect to the center of mass
/// \param m Total mass of the body
/// \param a Translation vector with respect to the center of mass
INLINE SymmetricTensor parallelAxisTheorem(const SymmetricTensor& I, const Float m, const Vector& a) {
    return I + m * (SymmetricTensor::identity() * getSqrLength(a) - outer(a, a));
}

} // namespace Rigid

/// \brief Returns the critical energy Q_D^* as a function of body diameter.
///
/// The critical energy is a kinetic energy for which half of the target is dispersed into the fragments. In
/// other words, impact with critical energy will produce largest remnant (or fragment), mass of which is 50%
/// mass of the parent body, The relation follows the scaling law of Benz & Asphaug (1999).
///
/// \todo replace D with units, do not enforce SI
INLINE Float evalBenzAsphaugScalingLaw(const Float D, const Float rho) {
    const Float D_cgs = 100._f * D;
    const Float rho_cgs = 1.e-3_f * rho;
    //  the scaling law parameters (in CGS units)
    constexpr Float Q_0 = 9.e7_f;
    constexpr Float B = 0.5_f;
    constexpr Float a = -0.36_f;
    constexpr Float b = 1.36_f;

    const Float Q_cgs = Q_0 * pow(D_cgs / 2._f, a) + B * rho_cgs * pow(D_cgs / 2._f, b);
    return 1.e-4_f * Q_cgs;
}

/// \brief Returns the the ratio of the cross-sectional area of the impact and the total area of the impactor.
///
/// This value is lower than 1 only at high impact angles, where a part of the impactor misses the target
/// entirely (hit-and-run collision) and does not deliver its kinetic energy into the target. For the same
/// Q/Q_D, oblique impacts will therefore look 'artificially' weaker than the impacts and lower impacts angle.
/// Effective impact area can be used to 'correct' the impact energy, so that impacts at high impact angles
/// are comparable with head-on impats.
/// See \ref Sevecek_2017 for details.
/// \param Q Relative impact energy (kinetic energy of the impactor divided by the impactor mass)
/// \param R Radius of the target
/// \param r Radius of the impactor
/// \param phi Impact angle in radians
INLINE Float getEffectiveImpactArea(const Float R, const Float r, const Float phi) {
    ASSERT(phi >= 0._f && phi <= PI / 2._f);
    const Float d = (r + R) * sin(phi);
    if (d < R - r) {
        return 1._f;
    } else {
        const Float area = sqr(r) * acos((sqr(d) + sqr(r) - sqr(R)) / (2 * d * r)) +
                           sqr(R) * acos((sqr(d) + sqr(R) - sqr(r)) / (2 * d * R)) -
                           0.5_f * sqrt((R + r - d) * (d + r - R) * (d - r + R) * (d + r + R));
        return area / (PI * sqr(r));
    }
}

enum class GetImpactorFlag {
    /// Use effective impact energy instead of 'regular' impact energy when determining impactor size
    EFFECTIVE_ENERGY = 1 << 0,
};

/// \brief Calculates the impactor radius to satisfy required impact parameters.
///
/// The radius is computed so that the total relative impact energy is equal to the given value, assuming
/// Benz & Asphaug scaling law.
/// \param R_pb Radius of the parent body (target)
/// \param v_imp Impact velocity
/// \param QoverQ_D Ratio of the impact energy and the critical energy given by the scaling law
/// \param rho Density of the parent bod
INLINE Float getImpactorRadius(const Float R_pb,
    const Float v_imp,
    const Float phi,
    const Float QoverQ_D,
    const Float rho,
    const Flags<GetImpactorFlag> flags) {
    if (flags.has(GetImpactorFlag::EFFECTIVE_ENERGY)) {
        // The effective impact energy depends on impactor radius, which is what we want to compute; needs to
        // be solved by iterative algorithm.
        // First, get an estimate of the radius by using the regular impact energy
        Float r = getImpactorRadius(R_pb, v_imp, phi, QoverQ_D, rho, EMPTY_FLAGS);
        if (almostEqual(getEffectiveImpactArea(R_pb, r, phi), 1._f, 1.e-4_f)) {
            // (almost) whole impactor hits the target, no need to account for effective energy
            return r;
        }
        // Effective energy LOWER than the regular energy, so we only need to increase the impactor, no need
        // to check for smaller values.
        Float lastR = LARGE;
        const Float eps = 1.e-4_f * r;
        while (abs(r - lastR) > eps) {
            lastR = r;
            const Float a = getEffectiveImpactArea(R_pb, r, phi);
            // effective->regular = divide by a
            r = getImpactorRadius(R_pb, v_imp, phi, QoverQ_D / a, rho, EMPTY_FLAGS);
        }
        return r;
    } else {
        // for 'regular' impact energy, simply compute the impactor radius by inverting Q=1/2 m_imp
        // v^2/M_pb
        const Float Q_D = evalBenzAsphaugScalingLaw(2._f * R_pb, rho);
        const Float Q = QoverQ_D * Q_D;
        return root<3>(2._f * Q / sqr(v_imp)) * R_pb;
    }
}


class ImpactCone {
private:
    AffineMatrix frame;
    Float v_c;

    UniformRng rng;

public:
    explicit ImpactCone(const AffineMatrix& frame, const Float cutoffVelocity)
        : frame(frame)
        , v_c(cutoffVelocity) {}

    /// \brief Generates fragments at the impact point.
    ///
    /// Particles are added into provided buffers, keeping the existing content untouched.
    /// \param m_tot Total mass of ejected fragments
    /// \param N Total number of fragments.
    /// \param r Output array of particle positions.
    /// \param r Output array of particle velocities.
    /// \param r Output array of particle masses.
    void getFragments(const Float m_tot, const Size N, Array<Vector>& r, Array<Vector>& v, Array<Float>& m) {
        constexpr Float theta = PI / 4._f;
        const Float m_frag = m_tot / N;
        for (Size i = 0; i < N; ++i) {
            const Float phi = 2._f * PI * rng();
            /// \todo sample velocity from power law
            v.push(frame * sphericalToCartesian(v_c, theta, phi));
            r.push(frame.translation());
            m.push(m_frag);
        }
    }
};

class CollisionMC {
private:
    UniformRng rng;

public:
    Array<Float> operator()(const Float QoverQ_D, const Float M_tot, const Float m_min) {
        const Float largest = max(M_LR(QoverQ_D, M_tot), M_LF(QoverQ_D, M_tot));
        const Float exponent = q(QoverQ_D) + 1._f;

        /// \todo
        Array<Float> fragments;
        fragments.push(largest);
        Float m_partial = largest;
        while (M_tot - m_partial > m_min) {
            const Float m = pow(rng(), 1._f / exponent) - m_min; /// \todo fix
            if (m + m_partial > M_tot) {
                continue;
            }
            fragments.push(m);
            m_partial += m;
        }
        return fragments;
    }

private:
    Float M_LR(const Float QoverQ_D, const Float M_tot) {
        if (QoverQ_D < 1._f) {
            return (-0.5_f * (QoverQ_D - 1._f) + 0.5_f) * M_tot;
        } else {
            return (-0.35_f * (QoverQ_D - 1._f) + 0.5_f) * M_tot;
        }
    }

    Float M_LF(const Float QoverQ_D, const Float M_tot) {
        return 8.e-3_f * (QoverQ_D * exp(-sqr(0.25_f * QoverQ_D))) * M_tot;
    }

    Float q(const Float QoverQ_D) {
        return -10._f + 7._f * pow(QoverQ_D, 0.4_f) * exp(-QoverQ_D / 7._f);
    }
};

NAMESPACE_SPH_END
