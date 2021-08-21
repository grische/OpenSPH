#pragma once

#include "run/Job.h"

NAMESPACE_SPH_BEGIN

class MaterialProvider {
protected:
    BodySettings body;

public:
    explicit MaterialProvider(const BodySettings& overrides = EMPTY_SETTINGS);

protected:
    void addMaterialEntries(VirtualSettings::Category& category, Function<bool()> enabler);
};

class MaterialJob : public IMaterialJob, public MaterialProvider {
public:
    explicit MaterialJob(const String& name, const BodySettings& overrides = EMPTY_SETTINGS);

    virtual String className() const override {
        return "material";
    }

    virtual UnorderedMap<String, ExtJobType> getSlots() const override {
        return {};
    }

    virtual VirtualSettings getSettings() override;

    virtual void evaluate(const RunSettings& global, IRunCallbacks& UNUSED(callbacks)) override;
};

class DisableDerivativeCriterionJob : public IMaterialJob {
public:
    explicit DisableDerivativeCriterionJob(const String& name)
        : IMaterialJob(name) {}

    virtual String className() const override {
        return "optimize timestepping";
    }

    virtual UnorderedMap<String, ExtJobType> getSlots() const override {
        return { { "material", JobType::MATERIAL } };
    }

    virtual VirtualSettings getSettings() override;

    virtual void evaluate(const RunSettings& global, IRunCallbacks& UNUSED(callbacks)) override;
};

NAMESPACE_SPH_END
