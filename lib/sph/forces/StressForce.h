#pragma once

#include "objects/wrappers/Flags.h"
#include "solvers/Accumulator.h"
#include "solvers/Module.h"
#include "storage/QuantityMap.h"
#include "storage/Storage.h"

NAMESPACE_SPH_BEGIN

/// Object computing acceleration of particles and increase of internal energy due to divergence of the stress
/// tensor. When no stress tensor is used in the model, only pressure gradient is computed.
template <typename Yielding, typename Damage, typename AV>
class StressForce : public Module<Yielding, Damage, AV, RhoDivv, RhoGradv> {
private:
    RhoDivv rhoDivv;
    RhoGradv rhoGradv;
    ArrayView<Float> p, rho, du, u, m, cs;
    ArrayView<Vector> v, dv;
    ArrayView<TracelessTensor> s, ds;

    enum class Options {
        USE_GRAD_P = 1 << 0,
        USE_DIV_S = 1 << 1,
    };
    Flags<Options> flags;

    Damage damage;
    Yielding yielding;
    AV av;

public:
    StressForce(const GlobalSettings& settings)
        : Module<Yielding, Damage, AV, RhoDivv, RhoGradv>(yielding, damage, av, rhoDivv, rhoGradv)
        , damage(settings)
        , av(settings) {
        flags.setIf(Options::USE_GRAD_P, settings.get<bool>(GlobalSettingsIds::MODEL_FORCE_GRAD_P));
        flags.setIf(Options::USE_DIV_S, settings.get<bool>(GlobalSettingsIds::MODEL_FORCE_DIV_S));
        // cannot use stress tensor without pressure
        ASSERT(!(flags.has(Options::USE_DIV_S) && !flags.has(Options::USE_GRAD_P)));
    }

    StressForce(const StressForce& other) = delete;

    StressForce(StressForce&& other) = default;

    void update(Storage& storage) {
        tieToArray(rho, u, m) = storage.getValues<Float>(QuantityKey::RHO, QuantityKey::U, QuantityKey::M);
        ArrayView<Vector> r;
        tieToArray(r, v, dv) = storage.getAll<Vector>(QuantityKey::R);
        if (flags.has(Options::USE_GRAD_P)) {
            p = storage.getValue<Float>(QuantityKey::P);
            cs = storage.getValue<Float>(QuantityKey::CS);
            // compute new values of pressure and sound speed
            for (int i = 0; i < r.size(); ++i) {
                tieToTuple(p[i], cs[i]) = storage.getMaterial(i).eos->getPressure(rho[i], u[i]);
            }
        }
        if (flags.has(Options::USE_DIV_S)) {
            tieToArray(s, ds) = storage.getAll<TracelessTensor>(QuantityKey::S);
        }
        this->updateModules(storage);
    }

    INLINE void accumulate(const int i, const int j, const Vector& grad) {
        Vector f(0._f);
        const Float rhoInvSqri = 1._f / Math::sqr(rho[i]);
        const Float rhoInvSqrj = 1._f / Math::sqr(rho[j]);
        if (flags.has(Options::USE_GRAD_P)) {
            /// \todo measure if these branches have any effect on performance
            const auto avij = av(i, j);
            f -= (reduce(p[i], i) * rhoInvSqri + reduce(p[j], i) * rhoInvSqrj + avij) * grad;
            // account for shock heating
            const Float heating = 0.5_f * avij * dot(v[i] - v[j], grad);
            du[i] += m[j] * heating;
            du[j] += m[i] * heating;
        }
        if (flags.has(Options::USE_DIV_S)) {
            f += (reduce(s[i], i) * rhoInvSqri + reduce(s[j], i) * rhoInvSqrj) * grad;
        }
        dv[i] += m[j] * f;
        dv[j] -= m[i] * f;
        // internal energy is computed at the end using accumulated values
        this->accumulateModules(i, j, grad);
    }

    void integrate(Storage& storage) {
        for (int i = 0; i < du.size(); ++i) {
            /// \todo check correct sign
            const Float rhoInvSqr = 1._f / Math::sqr(rho[i]);
            if (flags.has(Options::USE_GRAD_P)) {
                du[i] -= reduce(p[i], i) * rhoInvSqr * rhoDivv[i];
            }
            if (flags.has(Options::USE_DIV_S)) {
                du[i] += rhoInvSqr * ddot(reduce(s[i], i), rhoGradv[i]);

                // compute derivatives of the stress tensor
                /// \todo rotation rate tensor?
                const Float mu = storage.getMaterial(i).shearModulus;
                /// \todo how to enforce that this expression is traceless tensor?
                ds[i] += TracelessTensor(
                    2._f * mu * (rhoGradv[i] - Tensor::identity() * rhoGradv[i].trace() / 3._f));
                ASSERT(Math::isReal(ds[i]));
            }
            ASSERT(Math::isReal(du[i]));
        }
        this->integrateModules(storage);
    }

    void initialize(Storage& storage, const BodySettings& settings) const {
        storage.emplace<Float, OrderEnum::FIRST_ORDER>(QuantityKey::U,
            settings.get<Float>(BodySettingsIds::ENERGY),
            settings.get<Range>(BodySettingsIds::ENERGY_RANGE));
        if (flags.has(Options::USE_GRAD_P)) {
            // Compute pressure using equation of state
            std::unique_ptr<Abstract::Eos> eos = Factory::getEos(settings);
            const Float rho0 = settings.get<Float>(BodySettingsIds::DENSITY);
            const Float u0 = settings.get<Float>(BodySettingsIds::ENERGY);
            const int n = storage.getParticleCnt();
            Array<Float> p(n), cs(n);
            for (int i = 0; i < n; ++i) {
                tieToTuple(p[i], cs[i]) = eos->getPressure(rho0, u0);
            }
            storage.emplace<Float, OrderEnum::ZERO_ORDER>(QuantityKey::P, std::move(p));
            storage.emplace<Float, OrderEnum::ZERO_ORDER>(QuantityKey::CS, std::move(cs));
        }
        if (flags.has(Options::USE_DIV_S)) {
            storage.emplace<TracelessTensor, OrderEnum::FIRST_ORDER>(
                QuantityKey::S, settings.get<Float>(BodySettingsIds::STRESS_TENSOR));
        }
    }

private:
    /// \todo possibly precompute damage / yielding
    INLINE auto reduce(const Float pi, const int idx) const { return damage.reduce(pi, idx); }

    INLINE auto reduce(const TracelessTensor& si, const int idx) const {
        // first apply damage
        auto si_dmg = damage.reduce(si, idx);
        // then apply yielding using reduced stress
        return yielding.reduce(si_dmg, idx);
    }
};

NAMESPACE_SPH_END
