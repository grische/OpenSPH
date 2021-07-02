#pragma once

/// \file EquilibriumSolver.h
/// \brief Computes quantities to get equilibrium state
/// \author Pavel Sevecek
/// \date 2016-2021

#include "math/SparseMatrix.h"
#include "objects/wrappers/Outcome.h"
#include "sph/equations/EquationTerm.h"
#include "sph/solvers/SymmetricSolver.h"

NAMESPACE_SPH_BEGIN

class IGravity;

/// \brief Solver for equilibrium internal energy.
///
/// This assumes the only forces in the simulation are pressure gradient and gravity.
class EquilibriumEnergySolver {
    IScheduler& scheduler;

    AutoPtr<IGravity> gravity;

    AutoPtr<IBasicFinder> finder;

    SymmetrizeSmoothingLengths<LutKernel<3>> kernel;

    Size boundaryThreshold;

public:
    EquilibriumEnergySolver(IScheduler& scheduler,
        const RunSettings& settings,
        AutoPtr<IGravity>&& gravity,
        const Size boundaryThreshold);

    ~EquilibriumEnergySolver();

    Outcome solve(Storage& storage, Statistics& stats);
};

/// \brief Solves for total stress tensor sigma.
///
/// \note Equations to be solved cannot be specified at the moment, that would require a lot of extra work and
/// it is not needed. Will be possibly extended in the future.
class EquilibriumStressSolver {
private:
    IScheduler& scheduler;

    AutoPtr<ISymmetricFinder> finder;

    SymmetrizeSmoothingLengths<LutKernel<3>> kernel;

    SymmetricSolver<DIMENSIONS> equationSolver;

    Size boundaryThreshold;

    SparseMatrix matrix;

public:
    /// \brief Constructs the solver.
    ///
    /// \param equations Additional forces. The forces can depend on spatial derivatives, but must be
    ///                  independent on both pressure and deviatoric stress. All quantities appearing in these
    ///                  equations are considered parameters of the problem, solver cannot be used to solve
    ///                  other quantities than the total stress tensor.
    EquilibriumStressSolver(IScheduler& scheduler,
        const RunSettings& settings,
        const EquationHolder& equations);

    ~EquilibriumStressSolver();

    /// \brief Computed pressure and deviatoric stress are placed into the storage.
    ///
    /// This overrides previously stored values. Values of internal energy are computed using an equation of
    /// state.
    Outcome solve(Storage& storage, Statistics& stats);

    /// \brief Creates all the necessary quantities in the storage.
    void create(Storage& storage, IMaterial& material);
};

NAMESPACE_SPH_END
