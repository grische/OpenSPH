#pragma once

#include "run/Job.h"

NAMESPACE_SPH_BEGIN

#ifdef SPH_USE_CHAISCRIPT

class ChaiScriptJob : public IParticleJob {
private:
    Path file = Path("script.chai");

    int inputCnt = 8; // needs to be max for proper loading ...
    StaticArray<String, 8> slotNames;

    int paramCnt = 8;
    StaticArray<String, 8> paramNames;
    StaticArray<Float, 8> paramValues;

public:
    ChaiScriptJob(const String& name);

    virtual String className() const override {
        return "custom script";
    }

    virtual UnorderedMap<String, ExtJobType> getSlots() const override;

    virtual VirtualSettings getSettings() override;

    virtual void evaluate(const RunSettings& global, IRunCallbacks& UNUSED(callbacks)) override;
};

#endif

NAMESPACE_SPH_END
