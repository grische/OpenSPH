#pragma once

#include "objects/finders/NeighborFinder.h"
#include "sph/solvers/AsymmetricSolver.h"
#include "system/Statistics.h"

NAMESPACE_SPH_BEGIN

template <typename T>
class GravitySolver;

/// \brief Abstraction of the \f[f_{ij}\f] terms in \cite Owen_2009.
class IEnergyPartitioner : public Polymorphic {
public:
    virtual void initialize(const Storage& storage) = 0;

    virtual void compute(const Size i,
        ArrayView<const Size> neighs,
        ArrayView<const Float> e,
        ArrayView<Float> f) const = 0;
};

/// \brief Derivative holder, splitting the registered derivatives into accelerations and the rest.
class AccelerationSeparatingHolder : public DerivativeHolder {
private:
    FlatSet<RawPtr<IAcceleration>> accelerations;

public:
    virtual void require(AutoPtr<IDerivative>&& derivative) override {
        RawPtr<IAcceleration> a = dynamicCast<IAcceleration>(derivative.get());
        if (a) {
            accelerations.insert(a);
        }
        DerivativeHolder::require(std::move(derivative));
    }

    void evalAccelerations(const Size idx,
        ArrayView<const Size> neighs,
        ArrayView<const Vector> grads,
        Array<Vector>& dv) const {
        dv.fill(Vector(0._f));
        for (auto& a : accelerations) {
            a->evalAcceleration(idx, neighs, grads, dv);
        }
    }
};

/// See Owen 2009: A compatibly differenced total energy conserving form of SPH
class EnergyConservingSolver : public IAsymmetricSolver {
    friend class GravitySolver<EnergyConservingSolver>;

private:
    AccelerationSeparatingHolder derivatives;

    Float initialDt;

    AutoPtr<IEnergyPartitioner> partitioner;

    struct ThreadData {
        Array<NeighborRecord> neighs;

        /// \brief Holds the pair-wise changes of internal energy (Delta E_{thermal}) from the paper).
        Array<Float> energyChange;

        /// \brief Holds the energy change fraction for the given particle pair (f_{ij} from the paper).
        Array<Float> partitions;

        Array<Vector> accelerations;
    };

    ThreadLocal<ThreadData> threadData;

    Array<Array<Size>> neighList;

    Array<Array<Vector>> gradList;

public:
    EnergyConservingSolver(IScheduler& scheduler, const RunSettings& settings, const EquationHolder& eqs);

    EnergyConservingSolver(IScheduler& scheduler,
        const RunSettings& settings,
        const EquationHolder& eqs,
        AutoPtr<IBoundaryCondition>&& bc);

private:
    virtual void sanityCheck(const Storage& storage) const override;

    virtual void beforeLoop(Storage& storage, Statistics& stats) override;

    virtual void loop(Storage& storage, Statistics& stats) override;

    virtual void afterLoop(Storage& storage, Statistics& stats) override;
};

NAMESPACE_SPH_END
