#pragma once

/// \file Damage.h
/// \brief Models of fragmentation
/// \author Pavel Sevecek (sevecek at sirrah.troja.mff.cuni.cz)
/// \date 2016-2017

#include "common/ForwardDecl.h"
#include "objects/containers/ArrayView.h"
#include "objects/geometry/TracelessTensor.h"
#include "objects/geometry/Vector.h"
#include "objects/wrappers/Flags.h"

NAMESPACE_SPH_BEGIN

struct MaterialInitialContext;

enum class DamageFlag {
    PRESSURE = 1 << 0,         ///< Compute damaged values of pressure in place
    STRESS_TENSOR = 1 << 1,    ///< Compute damaged stress tensor and save them as quantity modification
    REDUCTION_FACTOR = 1 << 2, ///< Modify reduction factor (QuanityId::REDUCE) due to damage
};

namespace Abstract {
    class Damage : public Polymorphic {
    public:
        /// Sets up all the necessary quantities in the storage given material settings.
        virtual void setFlaws(Storage& storage,
            Abstract::Material& material,
            const MaterialInitialContext& context) const = 0;

        /// Computes modified values of given quantity due to fragmentation.
        virtual void reduce(Storage& storage, const Flags<DamageFlag> flags, const MaterialView sequence) = 0;

        /// Compute damage derivatives
        virtual void integrate(Storage& storage, const MaterialView sequence) = 0;
    };
}

enum class ExplicitFlaws {
    /// Distribute flaws uniformly (to random particles), see Benz & Asphaug (1994) \cite Benz_Asphaug_1994,
    /// Sec. 3.3.1
    UNIFORM,

    /// Explicitly assigned flaws by user, used mainly for testing purposes. Values must be set in quantity
    ASSIGNED,
};

/// Scalar damage describing fragmentation of the body according to Grady-Kipp model (Grady and Kipp, 1980)
class ScalarDamage : public Abstract::Damage {
private:
    Float kernelRadius;

    ExplicitFlaws options;

public:
    ScalarDamage(const Float kernelRadius, const ExplicitFlaws options = ExplicitFlaws::UNIFORM);

    ScalarDamage(const RunSettings& settings, const ExplicitFlaws options = ExplicitFlaws::UNIFORM);

    virtual void setFlaws(Storage& storage,
        Abstract::Material& material,
        const MaterialInitialContext& context) const override;

    virtual void reduce(Storage& storage,
        const Flags<DamageFlag> flags,
        const MaterialView material) override;

    virtual void integrate(Storage& storage, const MaterialView material) override;
};

class TensorDamage : public Abstract::Damage {
private:
public:
    virtual void setFlaws(Storage& storage,
        Abstract::Material& material,
        const MaterialInitialContext& context) const override;

    virtual void reduce(Storage& storage,
        const Flags<DamageFlag> flags,
        const MaterialView material) override;

    virtual void integrate(Storage& storage, const MaterialView material) override;
};

class NullDamage : public Abstract::Damage {
public:
    virtual void setFlaws(Storage& storage,
        Abstract::Material& material,
        const MaterialInitialContext& context) const override;

    virtual void reduce(Storage& storage,
        const Flags<DamageFlag> flags,
        const MaterialView material) override;

    virtual void integrate(Storage& storage, const MaterialView material) override;
};

NAMESPACE_SPH_END
