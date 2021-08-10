#include "sph/solvers/GravitySolver.h"
#include "gravity/SphericalGravity.h"
#include "objects/Exceptions.h"
#include "quantities/Quantity.h"
#include "sph/boundary/Boundary.h"
#include "sph/equations/Potentials.h"
#include "sph/kernel/Kernel.h"
#include "sph/solvers/AsymmetricSolver.h"
#include "sph/solvers/DensityIndependentSolver.h"
#include "sph/solvers/EnergyConservingSolver.h"
#include "sph/solvers/SymmetricSolver.h"
#include "system/Factory.h"
#include "system/Statistics.h"

NAMESPACE_SPH_BEGIN

template <typename TSphSolver>
GravitySolver<TSphSolver>::GravitySolver(IScheduler& scheduler,
    const RunSettings& settings,
    const EquationHolder& equations)
    : GravitySolver(scheduler, settings, equations, Factory::getBoundaryConditions(settings)) {}

template <typename TSphSolver>
GravitySolver<TSphSolver>::GravitySolver(IScheduler& scheduler,
    const RunSettings& settings,
    const EquationHolder& equations,
    AutoPtr<IBoundaryCondition>&& bc)
    : GravitySolver(scheduler, settings, equations, std::move(bc), Factory::getGravity(settings)) {}

template <typename TSphSolver>
GravitySolver<TSphSolver>::GravitySolver(IScheduler& scheduler,
    const RunSettings& settings,
    const EquationHolder& equations,
    AutoPtr<IBoundaryCondition>&& bc,
    AutoPtr<IGravity>&& gravity)
    : TSphSolver(scheduler, settings, equations, std::move(bc))
    , gravity(std::move(gravity)) {

    // make sure acceleration are being accumulated
    Accumulated& results = this->derivatives.getAccumulated();
    results.insert<Vector>(QuantityId::POSITION, OrderEnum::SECOND, BufferSource::SHARED);
}

template <>
GravitySolver<SymmetricSolver<DIMENSIONS>>::GravitySolver(IScheduler& scheduler,
    const RunSettings& settings,
    const EquationHolder& equations,
    AutoPtr<IBoundaryCondition>&& bc,
    AutoPtr<IGravity>&& gravity)
    : SymmetricSolver<DIMENSIONS>(scheduler, settings, equations, std::move(bc))
    , gravity(std::move(gravity)) {

    // make sure acceleration are being accumulated
    for (ThreadData& data : threadData) {
        Accumulated& results = data.derivatives.getAccumulated();
        results.insert<Vector>(QuantityId::POSITION, OrderEnum::SECOND, BufferSource::SHARED);
    }
}

template <typename TSphSolver>
GravitySolver<TSphSolver>::~GravitySolver() = default;

template <typename TSphSolver>
void GravitySolver<TSphSolver>::loop(Storage& storage, Statistics& stats) {
    VERBOSE_LOG

    // first, do asymmetric evaluation of gravity:

    // build gravity tree
    Timer timer;
    gravity->build(this->scheduler, storage);
    stats.set(StatisticsId::GRAVITY_BUILD_TIME, int(timer.elapsed(TimerUnit::MILLISECOND)));

    // get acceleration buffer corresponding to first thread (to save some memory + time)
    Accumulated& accumulated = this->getAccumulated();
    ArrayView<Vector> dv = accumulated.getBuffer<Vector>(QuantityId::POSITION, OrderEnum::SECOND);

    // evaluate gravity for each particle
    timer.restart();
    gravity->evalSelfGravity(this->scheduler, dv, stats);
    stats.set(StatisticsId::GRAVITY_EVAL_TIME, int(timer.elapsed(TimerUnit::MILLISECOND)));

    // evaluate gravity of attractors
    ArrayView<Attractor> attractors = storage.getAttractors();
    gravity->evalAttractors(this->scheduler, attractors, dv);

    // second, compute SPH derivatives using given solver
    timer.restart();
    TSphSolver::loop(storage, stats);
    stats.set(StatisticsId::SPH_EVAL_TIME, int(timer.elapsed(TimerUnit::MILLISECOND)));
}

template <>
Accumulated& GravitySolver<SymmetricSolver<DIMENSIONS>>::getAccumulated() {
    // gravity is evaluated asymmetrically, so we can simply put everything in the first (or any) accumulated
    ThreadData& data = this->threadData.value(0);
    return data.derivatives.getAccumulated();
}

template <>
Accumulated& GravitySolver<AsymmetricSolver>::getAccumulated() {
    return this->derivatives.getAccumulated();
}

template <>
Accumulated& GravitySolver<EnergyConservingSolver>::getAccumulated() {
    return this->derivatives.getAccumulated();
}

template <>
RawPtr<const IBasicFinder> GravitySolver<AsymmetricSolver>::getFinder(ArrayView<const Vector> r) {
    RawPtr<const IBasicFinder> finder = gravity->getFinder();
    if (!finder) {
        // no finder provided, just call the default implementation
        return AsymmetricSolver::getFinder(r);
    } else {
        return finder.get();
    }
}

template <>
RawPtr<const IBasicFinder> GravitySolver<EnergyConservingSolver>::getFinder(ArrayView<const Vector> r) {
    RawPtr<const IBasicFinder> finder = gravity->getFinder();
    if (!finder) {
        // no finder provided, just call the default implementation
        return EnergyConservingSolver::getFinder(r);
    } else {
        return finder.get();
    }
}

template <>
RawPtr<const IBasicFinder> GravitySolver<SymmetricSolver<DIMENSIONS>>::getFinder(
    ArrayView<const Vector> UNUSED(r)) {
    // Symmetric solver currently does not use this, we just implement it to make the templates work ...
    // If implemented, make sure to include RANK in the created tree - BarnesHut currently does not do that
    NOT_IMPLEMENTED;
}

template <typename TSphSolver>
void GravitySolver<TSphSolver>::sanityCheck(const Storage& storage) const {
    TSphSolver::sanityCheck(storage);
    // check that we don't solve gravity twice
    /// \todo generalize for ALL solvers of gravity (some categories?)
    if (this->equations.template contains<SphericalGravityEquation>()) {
        throw InvalidSetup(
            "Cannot use SphericalGravity in GravitySolver; only one solver of gravity is allowed");
    }
}

template class GravitySolver<SymmetricSolver<DIMENSIONS>>;
template class GravitySolver<AsymmetricSolver>;
// template class GravitySolver<DensityIndependentSolver>;
template class GravitySolver<EnergyConservingSolver>;

NAMESPACE_SPH_END
