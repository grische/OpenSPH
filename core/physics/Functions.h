#pragma once

/// \file Function.h
/// \brief Helper functions used by the code
/// \author Pavel Sevecek
/// \date 2016-2021

#include "math/rng/Rng.h"
#include "objects/geometry/SymmetricTensor.h"
#include "objects/wrappers/Flags.h"
#include "physics/Constants.h"

NAMESPACE_SPH_BEGIN

/// Contains analytic solutions of equations.
namespace Analytic {

/// \brief Properties of a homogeneous sphere in rest (no temporal derivatives)
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

    /// \brief Return the pressure at given radius r of a sphere self-compressed by gravity.
    INLINE Float getPressure(const Float r) const {
        if (r > r0) {
            return 0._f;
        }
        return 2._f / 3._f * PI * Constants::gravity * sqr(rho) * (sqr(r0) - sqr(r));
    }

    /// \brief Returns the gravitational acceleration at given radius r.
    ///
    /// The acceleration increases linearily up to r0 and then decreases with r^2.
    INLINE Vector getAcceleration(const Vector& r) const {
        const Float l = getLength(r);
        const Float l0 = min(r0, l);
        return -Constants::gravity * rho * sphereVolume(l0) * r / pow<3>(l);
    }

    /// \brief Returns the gravitational potential energy of the sphere.
    INLINE Float getEnergy() const {
        return -16._f / 15._f * sqr(PI) * sqr(rho) * Constants::gravity * pow<5>(r0);
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
    return I + m * (SymmetricTensor::identity() * getSqrLength(a) - symmetricOuter(a, a));
}

} // namespace Rigid

/// \brief Computes the critical (break-up) angular velocity of a body.
///
/// This value is only based on the ratio of the gravity and centrifugal force. It is a function of density
/// only, is does not depend on the radius of the body. Note that bodies rotating above the break-up limit
/// exist, especially smaller monolithic bodies with D ~ 100m!
/// \return Angular velocity (in rev/s).
INLINE Float getCriticalFrequency(const Float rho) {
    return sqrt(sphereVolume(1._f) * rho * Constants::gravity);
}

/// \brief Returns the critical energy Q_D^* as a function of body diameter.
///
/// The critical energy is a kinetic energy for which half of the target is dispersed into the fragments.
/// In other words, impact with critical energy will produce largest remnant (or fragment), mass of which
/// is 50% mass of the parent body, The relation follows the scaling law of Benz & Asphaug (1999).
///
/// \todo replace D with units, do not enforce SI
Float evalBenzAsphaugScalingLaw(const Float D, const Float rho);

/// \brief Computes the impact energy Q from impact parameters.
///
/// \param R Radius of the target
/// \param r Radius of the impactor
/// \param v Impact speed.
Float getImpactEnergy(const Float R, const Float r, const Float v);

/// \brief Returns the the ratio of the cross-sectional area of the impact and the total area of the
/// impactor.
///
/// This value is lower than 1 only at high impact angles, where a part of the impactor misses the target
/// entirely (hit-and-run collision) and does not deliver its kinetic energy into the target. For the same
/// Q/Q_D, oblique impacts will therefore look 'artificially' weaker than the impacts and lower impacts
/// angle. Effective impact area can be used to 'correct' the impact energy, so that impacts at high
/// impact angles are comparable with head-on impats. See \ref Sevecek_2017 for details.
/// \param Q Relative impact energy (kinetic energy of the impactor divided by the impactor mass)
/// \param R Radius of the target
/// \param r Radius of the impactor
/// \param phi Impact angle in radians
Float getEffectiveImpactArea(const Float R, const Float r, const Float phi);

/// \brief Calculates the impactor radius to satisfy required impact parameters.
///
/// The radius is computed so that the total relative impact energy is equal to the given value, assuming
/// Benz & Asphaug scaling law.
/// \param R_pb Radius of the parent body (target)
/// \param v_imp Impact velocity
/// \param QOverQ_D Ratio of the impact energy and the critical energy given by the scaling law
/// \param rho Density of the parent bod
Float getImpactorRadius(const Float R_pb, const Float v_imp, const Float QOverQ_D, const Float rho);

/// \brief Calculates the impactor radius to satisfy required impact parameters.
///
/// The radius is computed so that the total relative effective impact energy is equal to the given value,
/// assuming Benz & Asphaug scaling law. The effective impact energy takes into account the projected impact
/// area, see \ref getEffectiveImpactArea.
/// \param R_pb Radius of the parent body (target)
/// \param v_imp Impact velocity
/// \param phi Impact angle
/// \param Q_effOverQ_D Ratio of the relative impact energy and the critical energy given by the scaling law
/// \param rho Density of the parent bod
Float getImpactorRadius(const Float R_pb,
    const Float v_imp,
    const Float phi,
    const Float Q_effOverQ_D,
    const Float rho);


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
