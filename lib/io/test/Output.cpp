#include "io/Output.h"
#include "catch.hpp"
#include "io/Column.h"
#include "io/FileSystem.h"
#include <fstream>


using namespace Sph;


TEST_CASE("Dumping data", "[output]") {
    Storage storage;
    storage.insert<Vector>(
        QuantityId::POSITIONS, OrderEnum::SECOND, makeArray(Vector(0._f), Vector(1._f), Vector(2._f)));
    storage.insert<Float>(QuantityId::DENSITY, OrderEnum::FIRST, 5._f);
    TextOutput output("tmp%d.out", "Output", EMPTY_FLAGS);
    output.add(makeAuto<ValueColumn<Float>>(QuantityId::DENSITY));
    output.add(makeAuto<ValueColumn<Vector>>(QuantityId::POSITIONS));
    output.add(makeAuto<DerivativeColumn<Vector>>(QuantityId::POSITIONS));
    Statistics stats;
    stats.set(StatisticsId::TOTAL_TIME, 0._f);
    output.dump(storage, stats);

    std::string expected = R"(# Run: Output
# SPH dump, time = 0
#              Density        Position [x]        Position [y]        Position [z]        Velocity [x]        Velocity [y]        Velocity [z]
                   5                   0                   0                   0                   0                   0                   0
                   5                   1                   1                   1                   0                   0                   0
                   5                   2                   2                   2                   0                   0                   0
)";
    std::string content = readFile(Path("tmp0000.out"));
    REQUIRE(content == expected);
}
