#pragma once

/// \file IMaterial.h
/// \brief Base class for all particle materials
/// \author Pavel Sevecek (sevecek at sirrah.troja.mff.cuni.cz)
/// \date 2016-2021

#include "math/rng/Rng.h"
#include "objects/containers/FlatMap.h"
#include "objects/utility/IteratorAdapters.h"
#include "objects/wrappers/SharedPtr.h"
#include "quantities/QuantityIds.h"
#include "sph/initial/UvMapping.h"
#include "system/Settings.h"

NAMESPACE_SPH_BEGIN

class Storage;
class IMaterial;
class IScheduler;

/// \brief Non-owning wrapper of a material and particles with this material.
///
/// This object serves as a junction between particle storage and a material. It can be used to access
/// material parameters and member function, but also provides means to iterate over particle indices in the
/// storage.
///
/// Material accessed through MaterialView shares the constness of the view, i.e. material parameters cannot
/// be modified through const MaterialView.
class MaterialView {
private:
    RawPtr<IMaterial> mat;
    IndexSequence seq;

public:
    INLINE MaterialView(RawPtr<IMaterial>&& material, IndexSequence seq)
        : mat(std::move(material))
        , seq(seq) {
        SPH_ASSERT(material != nullptr);
    }

    /// Returns the reference to the material of the particles.
    INLINE IMaterial& material() {
        SPH_ASSERT(mat != nullptr);
        return *mat;
    }

    /// Returns the const reference to the material of the particles.
    INLINE IMaterial& material() const {
        SPH_ASSERT(mat != nullptr);
        return *mat;
    }

    /// Implicit conversion to the material.
    INLINE operator IMaterial&() {
        return this->material();
    }

    /// Implicit conversion to the material.
    INLINE operator const IMaterial&() const {
        return this->material();
    }

    /// Overloaded -> operator for convenient access to material functions.
    INLINE RawPtr<IMaterial> operator->() {
        return &this->material();
    }

    /// Overloaded -> operator for convenient access to material functions.
    INLINE RawPtr<const IMaterial> operator->() const {
        return &this->material();
    }

    /// Returns iterable index sequence.
    INLINE IndexSequence sequence() {
        return seq;
    }

    /// Returns iterable index sequence, const version.
    INLINE const IndexSequence sequence() const {
        return seq;
    }
};


/// \brief Shared data used when creating all bodies in the simulation.
///
/// \todo possibly generalize, allowing to create generic context as needed by components of the run
struct MaterialInitialContext {
    /// \brief Random number generator
    AutoPtr<IRng> rng;

    SharedPtr<IScheduler> scheduler;

    /// \brief Kernel radius in units of smoothing length.
    Float kernelRadius = 2._f;

    /// If used, texture mapping coordinates are generated provided mapping.
    AutoPtr<IUvMapping> uvMap = nullptr;

    MaterialInitialContext() = default;

    /// \brief Initializes the context using the values provided in the settings.
    explicit MaterialInitialContext(const RunSettings& settings);
};

/// \brief Material settings and functions specific for one material.
///
/// Contains all parameters needed during runtime that can differ for individual materials.
class IMaterial : public Polymorphic {
protected:
    /// Per-material parameters
    BodySettings params;

    /// Minimal values used in timestepping, do not affect values of quantities themselves.
    FlatMap<QuantityId, Float> minimals;

    /// Allowed range of quantities.
    FlatMap<QuantityId, Interval> ranges;

    /// Default values
    const static Interval DEFAULT_RANGE;
    const static Float DEFAULT_MINIMAL;

public:
    explicit IMaterial(const BodySettings& settings)
        : params(settings) {}

    template <typename TValue>
    INLINE IMaterial& setParam(const BodySettingsId paramIdx, TValue&& value) {
        params.set(paramIdx, std::forward<TValue>(value));
        return *this;
    }

    /// \brief Returns a parameter associated with given particle.
    template <typename TValue>
    INLINE TValue getParam(const BodySettingsId paramIdx) const {
        return params.get<TValue>(paramIdx);
    }

    /// \brief Returns settings containing material parameters.
    INLINE const BodySettings& getParams() const {
        return params;
    }

    /// \brief Sets the timestepping parameters of given quantity.
    ///
    /// Note that the function is not thread-safe and has to be synchronized when used in parallel
    /// context.
    void setRange(const QuantityId id, const Interval& range, const Float minimal);

    /// \brief Sets the timestepping parameters of given quantity.
    ///
    /// Syntactic sugar, using BodySettingsIds to retrieve the values from body settings of the material.
    INLINE void setRange(const QuantityId id, const BodySettingsId rangeId, const BodySettingsId minimalId) {
        this->setRange(id, params.get<Interval>(rangeId), params.get<Float>(minimalId));
    }

    /// \brief Returns the scale value of the quantity.
    ///
    /// This value is used by timestepping algorithms to determine the time step. The value can be
    /// specified by \ref setRange; if no value is specified, the fuction defaults to 0.
    INLINE Float minimal(const QuantityId id) const {
        return minimals.tryGet(id).valueOr(DEFAULT_MINIMAL);
    }

    /// \brief Returns the range of allowed quantity values.
    ///
    /// This range is enforced by timestetting algorithms, i.e. quantities do not have to be clamped by
    /// the solver or elsewhere. The range can be specified by \ref setRange; if no range is specified,
    /// the function defaults to unbounded interval.
    INLINE const Interval range(const QuantityId id) const {
        return ranges.tryGet(id).valueOr(DEFAULT_RANGE);
    }

    /// \brief Create all quantities needed by the material.
    virtual void create(Storage& storage, const MaterialInitialContext& context) = 0;

    /// \brief Initialize all quantities and material parameters.
    ///
    /// Called once every step before loop.
    /// \param scheduler Scheduler potentially used for parallelization
    /// \param storage Storage containing the particles and materials
    /// \param sequence Index sequence of the particles with this material
    virtual void initialize(IScheduler& scheduler, Storage& storage, const IndexSequence sequence) = 0;

    /// \brief Finalizes the material for the time step.
    ///
    /// Called after derivatives are computed.
    /// \param scheduler Scheduler potentially used for parallelization
    /// \param storage Storage containing the particles and materials
    /// \param sequence Index sequence of the particles with this material
    virtual void finalize(IScheduler& scheduler, Storage& storage, const IndexSequence sequence) = 0;
};


/// \brief Material that does not require any initialization or finalization.
///
/// It only holds the per-material properties of particles.
class NullMaterial : public IMaterial {
public:
    explicit NullMaterial(const BodySettings& body)
        : IMaterial(body) {}

    virtual void create(Storage& UNUSED(storage), const MaterialInitialContext& UNUSED(context)) override {}

    virtual void initialize(IScheduler& UNUSED(scheduler),
        Storage& UNUSED(storage),
        const IndexSequence UNUSED(sequence)) override {}

    virtual void finalize(IScheduler& UNUSED(scheduler),
        Storage& UNUSED(storage),
        const IndexSequence UNUSED(sequence)) override {}
};

NAMESPACE_SPH_END
