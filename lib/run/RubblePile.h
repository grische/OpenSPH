#pragma once

#include "run/Collision.h"
#include "sph/initial/Presets.h"

NAMESPACE_SPH_BEGIN

class RubblePileRunPhase : public IRunPhase {
private:
    CollisionParams collisionParams;
    PhaseParams phaseParams;

public:
    explicit RubblePileRunPhase(const CollisionParams& collisionParams, const PhaseParams& phaseParams);

    virtual void setUp() override;

    virtual void handoff(Storage&& input) override;

    virtual AutoPtr<IRunPhase> getNextPhase() const override;

private:
    virtual void tearDown(const Statistics& stats) override;
};

NAMESPACE_SPH_END
