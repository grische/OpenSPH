#pragma once

/// Standard artificial viscosity by Monaghan (1989), using a velocity divergence in linear and quadratic term
/// as a measure of local (scalar) dissipation. Parameters alpha_AV and beta_AV are constant (in time) and
/// equal for all particles.
/// Pavel Sevecek 2016
/// sevecek at sirrah.troja.mff.cuni.cz

#include "physics/Eos.h"
#include "quantities/Storage.h"
#include "solvers/EquationTerm.h"
#include "system/Settings.h"

NAMESPACE_SPH_BEGIN

class StandardAV : public Abstract::EquationTerm {
private:
    Float alpha, beta;

    class StandardAVDerivative : public Abstract::Derivative {
    private:
        ArrayView<const Vector> r, v;
        ArrayView<const Float> rho, cs, m;
        ArrayView<Vector> dv;
        const Float eps = 1.e-2_f;
        Float alpha, beta;

    public:
        StandardAVDerivative(const Float alpha, const Float beta)
            : alpha(alpha)
            , beta(beta) {}

        virtual void create(Accumulated& results) override {
            results.insert<Vector>(QuantityId::POSITIONS);
        }

        virtual void initialize(const Storage& input, Accumulated& results) override {
            ArrayView<const Vector> dummy;
            tie(r, v, dummy) = input.getAll<Vector>(QuantityId::POSITIONS);
            // sound speed must be computed by the solver using AV
            tie(rho, cs, m) =
                input.getValues<Float>(QuantityId::DENSITY, QuantityId::SOUND_SPEED, QuantityId::MASSES);
            dv = results.getValue<Vector>(QuantityId::POSITIONS);
        }

        virtual void compute(const Size i,
            ArrayView<const Size> neighs,
            ArrayView<const Vector> grads) override {
            ASSERT(neighs.size() == grads.size());
            for (Size k = 0; k < neighs.size(); ++k) {
                const Size j = neighs[k];
                const Float dvdr = dot(v[i] - v[j], r[i] - r[j]);
                if (dvdr >= 0._f) {
                    continue;
                }
                const Float hbar = 0.5_f * (r[i][H] + r[j][H]);
                const Float rhobar = 0.5_f * (rho[i] + rho[j]);
                const Float csbar = 0.5_f * (cs[i] + cs[j]);
                const Float mu = hbar * dvdr / (getSqrLength(r[i] - r[j]) + eps * sqr(hbar));
                const Vector Pi = 1._f / rhobar * (-alpha * csbar * mu + beta * sqr(mu)) * grads[k];
                ASSERT(isReal(Pi));
                /// \todo check sign
                dv[i] += m[j] * Pi;
                dv[j] -= m[i] * Pi;

                TODO("missing heating");
            }
        }
    };

public:
    StandardAV(const RunSettings& settings) {
        alpha = settings.get<Float>(RunSettingsId::SPH_AV_ALPHA);
        beta = settings.get<Float>(RunSettingsId::SPH_AV_BETA);
    }

    virtual void setDerivatives(DerivativeHolder& derivatives) override {
        derivatives.require<StandardAVDerivative>(alpha, beta);
    }

    virtual void finalize(Storage& UNUSED(storage)) override {}

    virtual void create(Storage& UNUSED_IN_RELEASE(storage),
        Abstract::Material& UNUSED(material)) const override {
        ASSERT(storage.has(QuantityId::SOUND_SPEED)); // doesn't make sense to use AV without pressure
    }
};


NAMESPACE_SPH_END
