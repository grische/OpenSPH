#pragma once

/// \file ISolver.h
/// \brief Base interface for all solvers
/// \author Pavel Sevecek (sevecek at sirrah.troja.mff.cuni.cz
/// \date 2016-2017

#include "common/ForwardDecl.h"

NAMESPACE_SPH_BEGIN

/// \brief Base class for all solvers.
///
/// This generic interface allows to use the code for any problems with explicit timestepping, meaning it is
/// SPH-agnostic. It can be used also for N-body simulations, etc. The solver computes derivatives of
/// time-dependent quantities and saves these derivatives to corresponding buffers in given \ref Storage
/// object. The temporal integration in then performed by time-stepping algorithm.
class ISolver : public Polymorphic {
public:
    /// \brief Computes derivatives of all time-dependent quantities.
    ///
    /// The solver can also modify the quantities arbitrarily. It is however not recommended to perform the
    /// integration in the solver (using the time step stored in \ref Statistics) as this is a job for
    /// timestepping. The solver can modify quantities using boundary conditions, inter-quentity relationships
    /// (such as the summation equation for density in SPH), clamping of values etc. It is also possible to
    /// add or remove particles in the storage and modify materials. Threads running concurrently with the
    /// solver must assume the solver can modify the storage at any time, so accessing the storage from
    /// different threads is only allowed before or after \ref ISolver::integrate; the is no locking for
    /// performance reasons.
    ///
    /// All highest order derivatives are guaranteed to be set to zero when \ref ISolver::integrate is called
    /// (this is responsibility of \ref ITimeStepping implementation).
    ///
    /// \param storage Storage containing all quantities.
    /// \param stats Object where the solver saves all computes statistics of the run.
    virtual void integrate(Storage& storage, Statistics& stats) = 0;

    /// \brief Initializes all quantities needed by the solver in the storage.
    ///
    /// When called, storage already contains particle positions and their masses. All remaining quantities
    /// must be created by the solver. The function is called once for every body in the run. The given
    /// storage is guaranteed to be homogeneous; it contains only a single material.
    ///
    /// Note that when settings up initial condition using \ref InitialConditions object, the instance of \ref
    /// ISolver used for creating quantities can be different than the one used during the run. It is not
    /// recommended to set up mutable member variables of the solver from \ref ISolver::create function or
    /// keep a reference to the calling solver elsewhere.
    ///
    /// \param storage Particle storage that shall be modified as needed by the solver.
    /// \param material Material containing parameters of the body being created. The solver can also set up
    ///                 necessary timestepping parameters of the material (ranges and minimal values of
    ///                 quantities).
    virtual void create(Storage& storage, IMaterial& material) const = 0;
};


NAMESPACE_SPH_END
