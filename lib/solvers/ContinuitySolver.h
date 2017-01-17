#pragma once

/// Standard SPH solver, using density and specific energy as independent variables. Evolves density using
/// continuity equation and energy using energy equation. Works with any artificial viscosity and any equation
/// of state.
/// Pavel Sevecek 2016
/// sevecek at sirrah.troja.mff.cuni.cz

#include "objects/containers/ArrayUtils.h"
#include "objects/finders/AbstractFinder.h"
#include "quantities/Iterate.h"
#include "quantities/QuantityIds.h"
#include "solvers/AbstractSolver.h"
#include "sph/av/Standard.h"
#include "sph/boundary/Boundary.h"
#include "sph/forces/StressForce.h"
#include "sph/kernel/Kernel.h"
#include "system/Factory.h"
#include "system/Profiler.h"
#include "system/Settings.h"

NAMESPACE_SPH_BEGIN

template <typename Force, int D>
class ContinuitySolver : public SolverBase<D>, Module<Force, RhoDivv> {
private:
    Force force;

    static constexpr int dim = D;

    /// \todo SharedAccumulator
    RhoDivv rhoDivv;

public:
    ContinuitySolver(const GlobalSettings& settings)
        : SolverBase<D>(settings)
        , Module<Force, RhoDivv>(force, rhoDivv)
        , force(settings) {}

    virtual void integrate(Storage& storage) override {
        const Size size = storage.getParticleCnt();
        ArrayView<Vector> r, v, dv;
        ArrayView<Float> m, rho, drho;
        {
            PROFILE_SCOPE("ContinuitySolver::compute (getters)")
            tie(r, v, dv) = storage.getAll<Vector>(QuantityIds::POSITIONS);
            tie(rho, drho) = storage.getAll<Float>(QuantityIds::DENSITY);
            m = storage.getValue<Float>(QuantityIds::MASSES);
            // Check that quantities are valid
            ASSERT(areAllMatching(dv, [](const Vector v) { return v == Vector(0._f); }));
            ASSERT(areAllMatching(rho, [](const Float v) { return v > 0._f; }));

            // clamp smoothing length
            for (Float& h : componentAdapter(r, H)) {
                h = max(h, 1.e-12_f);
            }
        }
        {
            PROFILE_SCOPE("ContinuitySolver::compute (updateModules)")
            this->updateModules(storage);
        }
        {
            PROFILE_SCOPE("ContinuitySolver::compute (build)")
            this->finder->build(r);
        }
        // we symmetrize kernel by averaging smoothing lenghts
        SymH<dim> w(this->kernel);
        for (Size i = 0; i < size; ++i) {
            // Find all neighbours within kernel support. Since we are only searching for particles with
            // smaller h, we know that symmetrized lengths (h_i + h_j)/2 will be ALWAYS smaller or equal to
            // h_i, and we thus never "miss" a particle.
            this->finder->findNeighbours(i,
                r[i][H] * this->kernel.radius(),
                this->neighs,
                FinderFlags::FIND_ONLY_SMALLER_H | FinderFlags::PARALLELIZE);
            // iterate over neighbours
            PROFILE_SCOPE("ContinuitySolver::compute (iterate)")
            for (const auto& neigh : this->neighs) {
                const int j = neigh.index;
                // actual smoothing length
                const Float hbar = 0.5_f * (r[i][H] + r[j][H]);
                ASSERT(hbar > EPS && hbar <= r[i][H]);
                if (getSqrLength(r[i] - r[j]) > sqr(this->kernel.radius() * hbar)) {
                    // aren't actual neighbours
                    continue;
                }
                // compute gradient of kernel W_ij
                const Vector grad = w.grad(r[i], r[j]);
                ASSERT(isReal(grad) && dot(grad, r[i] - r[j]) <= 0._f);

                this->accumulateModules(i, j, grad);
            }
        }

        // set derivative of density and smoothing length
        for (Size i = 0; i < drho.size(); ++i) {
            drho[i] = -rhoDivv[i];
            v[i][H] = r[i][H] / (D * rho[i]) * rhoDivv[i];
            dv[i][H] = 0._f;
        }
        this->integrateModules(storage);
        if (this->boundary) {
            PROFILE_SCOPE("ContinuitySolver::compute (boundary)")
            this->boundary->apply(storage);
        }
    }

    virtual void initialize(Storage& storage, const BodySettings& settings) const override {
        storage.emplace<Float, OrderEnum::FIRST_ORDER>(QuantityIds::DENSITY,
            settings.get<Float>(BodySettingsIds::DENSITY),
            settings.get<Range>(BodySettingsIds::DENSITY_RANGE),
            settings.get<Float>(BodySettingsIds::DENSITY_MIN));
        this->initializeModules(storage, settings);
    }
};

NAMESPACE_SPH_END
