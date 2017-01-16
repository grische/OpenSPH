#include "physics/Yielding.h"
#include "catch.hpp"
#include "quantities/Material.h"
#include "quantities/Storage.h"
#include "system/Settings.h"

using namespace Sph;


TEST_CASE("VonMises", "[yielding]") {
    VonMises vonMises;
    Storage storage(BODY_SETTINGS);
    Array<Float> energy(10);
    energy.fill(0._f);
    storage.emplace<Float, OrderEnum::FIRST_ORDER>(QuantityIds::ENERGY, std::move(energy));
    MaterialAccessor material(storage);
    material.setParams(BodySettingsIds::MELT_ENERGY, 100._f);
    material.setParams(BodySettingsIds::ELASTICITY_LIMIT, BODY_SETTINGS);
    TracelessTensor s(Vector(1._f, 2._f, 3._f), Vector(2._f, 2._f, 4._f), Vector(3._f, 4._f, -3._f));

    vonMises.update(storage);
    REQUIRE(vonMises.reduce(s, 0) == s); // small stress, elastic material

    Array<Float>& u = storage.getValue<Float>(QuantityIds::ENERGY);
    u.fill(120._f);
    vonMises.update(storage);
    REQUIRE(vonMises.reduce(s, 0) == TracelessTensor::null()); // u = u_melt, no stress
}
