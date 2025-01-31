#include "system/ArrayStats.h"
#include "catch.hpp"
#include "objects/containers/Array.h"
#include "tests/Approx.h"

using namespace Sph;

TEST_CASE("Statistics", "[statistics]") {
    Array<Float> values{ 13._f, 18._f, 13._f, 14._f, 13._f, 16._f, 14._f, 21._f, 13._f };
    ArrayStats<Float> stats(values);
    REQUIRE(stats.min() == 13._f);
    REQUIRE(stats.max() == 21._f);
    REQUIRE(stats.median() == 14._f);
    REQUIRE(stats.average() == approx(15._f));
}
