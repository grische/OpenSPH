#include "models/BasicModel.h"
#include "catch.hpp"
#include "geometry/Domain.h"
#include "objects/containers/ArrayUtils.h"
#include "objects/wrappers/Range.h"
#include "sph/timestepping/TimeStepping.h"
#include "physics/Integrals.h"
#include "physics/Constants.h"

using namespace Sph;

TEST_CASE("create particles", "[basicmodel]") {
    BasicModel model(std::make_shared<Storage>(), GLOBAL_SETTINGS);
    Storage storage =
        model.createParticles(100, std::make_unique<BlockDomain>(Vector(0._f), Vector(1._f)), BODY_SETTINGS);


    const int size = storage.get<QuantityKey::R>().size();
    REQUIRE(Range<int>(80, 120).contains(size));
    iterate<TemporalEnum::ALL>(storage, [size](auto&& array) { REQUIRE(array.size() == size); });

    ArrayView<Float> rhos, us, drhos, dus;
    tie(rhos, us)   = storage.get<QuantityKey::RHO, QuantityKey::U>();
    tie(drhos, dus) = storage.dt<QuantityKey::RHO, QuantityKey::U>();
    bool result = areAllMatching(rhos, [](const Float f) {
        return f == 2700._f; // density of 2700km/m^3
    });
    REQUIRE(result);
    /* derivatives can be nonzero here, they must be cleaned in timestepping::init()
     * result = areAllMatching(drhos, [](const Float f){
        return f == 0._f; // zero density derivative
    });
    REQUIRE(result);*/
    result = areAllMatching(us, [](const Float f) {
        return f == 0._f; // zero internal energy
    });
    REQUIRE(result);
    /*result = areAllMatching(dus, [](const Float f){
        return f == 0._f; // zero energy derivative
    });
    REQUIRE(result);*/
}

TEST_CASE("simple run", "[basicmodel]") {
    Settings<GlobalSettingsIds> globalSettings(GLOBAL_SETTINGS);
    globalSettings.set<Float>(GlobalSettingsIds::TIMESTEPPING_INITIAL_TIMESTEP, 1.e-4_f);
    globalSettings.set<int>(GlobalSettingsIds::FINDER, int(FinderEnum::BRUTE_FORCE));
    std::shared_ptr<Storage> storage = std::make_shared<Storage>();
    BasicModel model(storage, globalSettings);

    // set initial energy to nonzero value, to get some pressure
    Settings<BodySettingsIds> bodySettings(BODY_SETTINGS);
    bodySettings.set<Float>(BodySettingsIds::ENERGY, 100._f * Constants::gasConstant); // 100K
    *storage =
        model.createParticles(100, std::make_unique<BlockDomain>(Vector(0._f), Vector(1._f)), bodySettings);


    EulerExplicit timestepping(storage, globalSettings);
    // check integrals of motion
    TotalMomentum momentum(storage);
    TotalAngularMomentum angularMomentum(storage);
    const Vector mom0 = momentum.compute();
    const Vector angmom0 = angularMomentum.compute();
    REQUIRE(mom0 == Vector(0._f));
    REQUIRE(angmom0 == Vector(0._f));
    for (float t = 0._f; t < 1._f; t += timestepping.getTimeStep()) {
        timestepping.step(&model);
    }
    const Vector mom1 = momentum.compute();
    const Vector angmom1 = angularMomentum.compute();
    REQUIRE(mom1 == Vector(0._f));
    REQUIRE(angmom1 == Vector(0._f));

    // check that particles gained some velocity
    Float totV = 0._f;
    for (Vector& v : storage->dt<QuantityKey::R>()) {
        totV += getLength(v);
    }
    REQUIRE(totV > 0._f);
}
