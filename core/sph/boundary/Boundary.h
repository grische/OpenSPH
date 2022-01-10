#pragma once

/// \file Boundary.h
/// \brief Boundary conditions
/// \author Pavel Sevecek (sevecek at sirrah.troja.mff.cuni.cz)
/// \date 2016-2021

#include "common/ForwardDecl.h"
#include "objects/containers/Array.h"
#include "objects/geometry/Domain.h"
#include "objects/geometry/Vector.h"
#include "objects/wrappers/AutoPtr.h"
#include "objects/wrappers/Interval.h"
#include "quantities/Storage.h"
#include <set>

NAMESPACE_SPH_BEGIN

class ISymmetricFinder;

/// \brief Base object for boundary conditions.
///
/// All boundary conditions behave similarly to equation terms, meaning they can modify the storage each time
/// step before the derivatives are computed (in \ref initialize function) and after derivatives are computed
/// (in \ref finalize function).
/// They do not create any quantities nor they compute derivatives. In solver, boundary conditions are
/// evaluated after all the equations have been evaluated. This is needed in order to copy the correct
/// quantities on ghosts, make sure that derivatives are zero/clamped as needed, etc.
class IBoundaryCondition : public Polymorphic {
public:
    /// \brief Applies the boundary conditions before the derivatives are computed.
    ///
    /// Called every time step after equations are initialized.
    virtual void initialize(Storage& storage) = 0;

    /// \brief Applies the boundary conditions after the derivatives are computed.
    ///
    /// Called every time step after equations are evaluated (finalized).
    virtual void finalize(Storage& storage) = 0;
};

struct NullBoundaryCondition : public IBoundaryCondition {
    virtual void initialize(Storage& UNUSED(storage)) override {}
    virtual void finalize(Storage& UNUSED(storage)) override {}
};

/// \brief Provides a way to access ghost particle data outside the solver.
class GhostParticlesData : public IStorageUserData {
private:
    Array<Ghost> ghosts;

public:
    explicit GhostParticlesData(Array<Ghost>&& ghosts)
        : ghosts(std::move(ghosts)) {}

    /// \brief Returns the reference to ghost particle with given index.
    Ghost& getGhost(const Size idx) {
        return ghosts[idx];
    }

    /// \brief Returns the total number of ghost particles.
    Size size() const {
        return ghosts.size();
    }

    virtual void remove(ArrayView<const Size> idxs) override;
};

/// \brief Adds ghost particles symmetrically for each SPH particle close to boundary.
///
/// All physical quantities are copied on them. This acts as a natural boundary for SPH particles without
/// creating unphysical gradients due to discontinuity.
class GhostParticles : public IBoundaryCondition {
public:
    /// Special flag denoting ghost particles.
    static constexpr Size FLAG = Size(-1);

private:
    Array<Ghost> ghosts;
    Array<Size> ghostIdxs;
    SharedPtr<IDomain> domain;

    /// Parameters of the BCs
    struct {
        Float searchRadius;
        Float minimalDist;
    } params;

    /// Optional; specifies a functor returning a velocity assigned to given ghost.
    Function<Optional<Vector>(const Vector& r)> ghostVelocity;

    /// Used as consistency check; currently we don't allow any other object to add or remove particles if
    /// GhostParticles are used.
    Size particleCnt;

public:
    /// \brief Creates new boundary conditions.
    /// \param domain Selected computational domain; particles close to the boundary will spawn ghosts.
    /// \param searchRadius Radius of a single particle, in units of smoothing length.
    /// \param minimalDist Minimal allowed distance between a particle and its ghost.
    GhostParticles(SharedPtr<IDomain> domain, const Float searchRadius, const Float minimalDist);

    /// \brief Creates new boundary conditions, using parameters specified in settings.
    GhostParticles(SharedPtr<IDomain> domain, const RunSettings& settings);

    /// \brief Specifies a functor that overrides the default velocity assinged to each ghost.
    ///
    /// By default, the velocity of a ghost is mirrored with a respect to the boundary; the tangential
    /// conponents of the velocities of the particle and the corresponding ghost are equal. The provided
    /// function may return a different velocity to obtain different boundary condition at different parts of
    /// the boundary, for example it may return zero velocity to create friction between the particles and the
    /// boundary.
    /// \param ghostVelocity Functor taking ghost position as parameter and returning velocity assigned to
    ///                      the ghost or NOTHING to assign the default velocity.
    void setVelocityOverride(Function<Optional<Vector>(const Vector& r)> ghostVelocity);

    virtual void initialize(Storage& storage) override;

    virtual void finalize(Storage& storage) override;
};

/// \brief Places immovable particles along boundary.
///
/// The dummy particles can have generally different material than the rest of the simulation.
class FixedParticles : public IBoundaryCondition {
public:
    struct Params {
        /// Computational domain; dummy particles will be placed outside of the domain
        AutoPtr<IDomain> domain;

        /// Distribution used to create the dummy particles
        AutoPtr<IDistribution> distribution;

        /// Material of the dummy particles. This can be generally different than the material of "real"
        /// particles in the simulation, specifically the material is not copied as for ghost particles.
        AutoPtr<IMaterial> material;

        /// Maximum size of the boundary layer. Note that this is an absolute value, it is NOT in units of
        /// smoothing length.
        Float thickness = NAN;
    };

private:
    /// Storage containing the fixed particles.
    Storage fixedParticles;

public:
    FixedParticles(const RunSettings& settings, Params&& params);

    virtual void initialize(Storage& storage) override;

    virtual void finalize(Storage& storage) override;
};


/// \brief Boundary condition that nulls all highest derivates of selected particles.
///
/// Term can freeze particles close to the boundary and/or particles of given body or list of bodies. This
/// will cause the particles to keep quantity values given by their initial conditions and move with initial
/// velocity. The 'frozen' particles affect other particles normally and contribute to all integrals, such as
/// total mometum or energy.
class FrozenParticles : public IBoundaryCondition {
protected:
    SharedPtr<IDomain> domain;
    Float radius;

    std::set<Size> frozen;

    Array<Float> distances;
    Array<Size> idxs;

public:
    /// \brief Constructs boundary conditions with no particles frozen.
    ///
    /// These can be later added using freeze function.
    FrozenParticles();

    ~FrozenParticles();

    /// \brief Constructs boundary conditions with frozen particles near boundary.
    ///
    /// \param domain Domain defining the boundary
    /// \param radius Search radius (in units of smoothing length) up to which the particles will be frozen.
    FrozenParticles(SharedPtr<IDomain> domain, const Float radius);

    /// \brief Adds body ID particles of which shall be frozen by boundary conditions.
    void freeze(const Size flag);

    /// \brief Remove a body from the list of frozen bodies.
    ///
    /// If the body is not on the list, nothing happens.
    void thaw(const Size flag);

    virtual void initialize(Storage& UNUSED(storage)) override {}

    virtual void finalize(Storage& storage) override;
};


/// \brief Boundary conditions creating particles with given velocity at the boundary of the domain.
///
/// Outgoing particles are removed at the other side of the boundary, particles inside the domain are kept in
/// place using either ghost particles or by freezing them. This is fine-tuned for simulations of a meteorite
/// passing through athmosphere.
class WindTunnel : public FrozenParticles {
public:
    WindTunnel(SharedPtr<IDomain> domain, const Float radius);

    virtual void initialize(Storage& UNUSED(storage)) override {}

    virtual void finalize(Storage& storage) override;
};

/// \brief Boundary condition moving all particles passed through the domain to the other side of the domain.
class PeriodicBoundary : public IBoundaryCondition {
private:
    Box domain;

    Array<Ghost> ghosts;
    Array<Size> ghostIdxs;

public:
    explicit PeriodicBoundary(const Box& domain);

    virtual void initialize(Storage& storage) override;

    virtual void finalize(Storage& storage) override;
};

/// \brief Boundary duplicating particles along z=0 plane.
///
/// Particles from z>0 half-space are copied into the z<0 half-space. Any particles originally located in the
/// negative half-space are first projected to the z=0 plane.
class SymmetricBoundary : public IBoundaryCondition {
private:
    Array<Ghost> ghosts;
    Array<Size> ghostIdxs;

public:
    virtual void initialize(Storage& storage) override;

    virtual void finalize(Storage& storage) override;
};

/// \brief Boundary condition that removes particles outside the domain.
class KillEscapersBoundary : public IBoundaryCondition {
private:
    SharedPtr<IDomain> domain;

public:
    explicit KillEscapersBoundary(SharedPtr<IDomain> domain);

    virtual void initialize(Storage& storage) override;

    virtual void finalize(Storage& storage) override;
};

/// Helper tool for 1D tests, projects all particles onto a 1D line.
class Projection1D : public IBoundaryCondition {
private:
    Interval domain;
    ArrayView<Vector> r, v;

public:
    /// Constructs using range as 1D domain
    explicit Projection1D(const Interval& domain);

    virtual void initialize(Storage& UNUSED(storage)) override {}

    virtual void finalize(Storage& storage) override;
};


NAMESPACE_SPH_END
