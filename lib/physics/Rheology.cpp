#include "physics/Rheology.h"
#include "physics/Damage.h"
#include "quantities/AbstractMaterial.h"
#include "quantities/Storage.h"

NAMESPACE_SPH_BEGIN

VonMisesRheology::VonMisesRheology()
    : VonMisesRheology(makeAuto<NullDamage>()) {}

VonMisesRheology::VonMisesRheology(AutoPtr<Abstract::Damage>&& damage)
    : damage(std::move(damage)) {
    ASSERT(this->damage != nullptr);
}

VonMisesRheology::~VonMisesRheology() = default;

void VonMisesRheology::create(Storage& storage,
    const BodySettings& settings,
    const MaterialInitialContext& context) const {
    ASSERT(storage.getMaterialCnt() == 1);
    storage.insert<Float>(QuantityId::STRESS_REDUCING, OrderEnum::ZERO, 1._f);
    damage->setFlaws(storage, settings, context);
}

void VonMisesRheology::initialize(Storage& storage, const MaterialView material) {
    ArrayView<Float> u = storage.getValue<Float>(QuantityId::ENERGY);
    ArrayView<Float> reducing = storage.getValue<Float>(QuantityId::STRESS_REDUCING);

    // reduce stress tensor by damage
    damage->reduce(storage, DamageFlag::PRESSURE | DamageFlag::STRESS_TENSOR, material);
    ArrayView<TracelessTensor> S, S_dmg;
    tie(S, S_dmg) = storage.modify<TracelessTensor>(QuantityId::DEVIATORIC_STRESS);

    const Float limit = material->getParam<Float>(BodySettingsId::ELASTICITY_LIMIT);
    const Float u_melt = material->getParam<Float>(BodySettingsId::MELT_ENERGY);
    IndexSequence seq = material.sequence();
    constexpr Float eps = 1.e-15_f;
    storage.parallelFor(*seq.begin(), *seq.end(), [&](const Size n1, const Size n2) INL {
        // compute yielding stress
        for (Size i = n1; i < n2; ++i) {
            const Float unorm = u[i] / u_melt;
            const Float y = unorm < 1.e-5_f ? limit : limit * max(1.f - unorm, 0._f);
            ASSERT(limit > 0._f);

            // apply reduction to stress tensor
            if (y < EPS) {
                reducing[i] = 0._f;
                S[i] = TracelessTensor::null();
                continue;
            }
            // compute second invariant using damaged stress tensor
            const TracelessTensor s = S_dmg[i] + TracelessTensor(eps);
            const TracelessTensor sy = s / y + TracelessTensor(eps);
            const Float inv = 0.5_f * ddot(sy, sy) + eps;
            ASSERT(isReal(inv) && inv > 0._f);
            const Float red = min(sqrt(1._f / (3._f * inv)), 1._f);
            ASSERT(red >= 0._f && red <= 1._f);
            reducing[i] = red;

            // apply yield reduction in place
            S[i] = S[i] * red;
            ASSERT(isReal(S[i]));
        }
    });

    // now we have to compute new values of damage stress tensor
    damage->reduce(storage, DamageFlag::STRESS_TENSOR | DamageFlag::REDUCTION_FACTOR, material);
}

void VonMisesRheology::integrate(Storage& storage, const MaterialView material) {
    damage->integrate(storage, material);
}

DruckerPragerRheology::DruckerPragerRheology(AutoPtr<Abstract::Damage>&& damage)
    : damage(std::move(damage)) {
    if (this->damage == nullptr) {
        this->damage = makeAuto<NullDamage>();
    }
}

DruckerPragerRheology::~DruckerPragerRheology() = default;

void DruckerPragerRheology::create(Storage& storage,
    const BodySettings& settings,
    const MaterialInitialContext& context) const {
    ASSERT(storage.getMaterialCnt() == 1);
    /// \todo implement, this is copy+paste of von mises
    storage.insert<Float>(QuantityId::STRESS_REDUCING, OrderEnum::ZERO, 1._f);
    damage->setFlaws(storage, settings, context);
}

void DruckerPragerRheology::initialize(Storage& storage, const MaterialView material) {
    yieldingStress.clear();
    /// ArrayView<Float> u = storage.getValue<Float>(QuantityId::ENERGY); \todo dependence on melt energy
    ArrayView<Float> p = storage.getValue<Float>(QuantityId::PRESSURE);
    ArrayView<Float> D = storage.getValue<Float>(QuantityId::DAMAGE);
    for (Size i : material.sequence()) {
        const Float Y_0 = material->getParam<Float>(BodySettingsId::COHESION);
        const Float mu_i = material->getParam<Float>(BodySettingsId::INTERNAL_FRICTION);
        const Float Y_M = material->getParam<Float>(BodySettingsId::ELASTICITY_LIMIT);
        // const Float u_melt = storage.getParam<Float>(BodySettingsId::MELT_ENERGY);
        const Float Y_i = Y_0 + mu_i * p[i] / (1._f + mu_i * p[i] / (Y_M - Y_0));
        ASSERT(Y_i >= 0);

        const Float mu_d = material->getParam<Float>(BodySettingsId::DRY_FRICTION);
        const Float Y_d = mu_d * p[i];
        if (Y_d > Y_i) {
            // at pressures above Y_i, the shear strength follows the same pressure dependence regardless
            // of damage.
            yieldingStress.push(Y_i);
        } else {
            const Float d = pow<3>(D[i]);
            const Float Y = (1._f - d) * Y_i + d * Y_d;
            yieldingStress.push(Y);
        }
    }
}

void DruckerPragerRheology::integrate(Storage& storage, const MaterialView material) {
    damage->integrate(storage, material);
}


void ElasticRheology::create(Storage& storage,
    const BodySettings& UNUSED(settings),
    const MaterialInitialContext& UNUSED(context)) const {
    ASSERT(storage.getMaterialCnt() == 1);
    storage.insert<Float>(QuantityId::STRESS_REDUCING, OrderEnum::ZERO, 1._f);
}

void ElasticRheology::initialize(Storage& UNUSED(storage), const MaterialView UNUSED(material)) {}

void ElasticRheology::integrate(Storage& UNUSED(storage), const MaterialView UNUSED(material)) {}

NAMESPACE_SPH_END
