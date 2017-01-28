#include "sph/initial/Distribution.h"
#include "catch.hpp"
#include "geometry/Domain.h"
#include "objects/containers/ArrayUtils.h"
#include "objects/finders/AbstractFinder.h"
#include "system/ArrayStats.h"
#include "system/Factory.h"
#include "system/Output.h"
#include "utils/Utils.h"
#include "utils/Approx.h"
#include "utils/SequenceTest.h"

using namespace Sph;

void testDistribution(Abstract::Distribution* distribution) {
    BlockDomain domain(Vector(-3._f), Vector(2._f));
    Array<Vector> values = distribution->generate(1000, domain);
    // distribution generates approximately 1000 particles
    REQUIRE(values.size() > 900);
    REQUIRE(values.size() < 1100);

    // all particles are inside prescribed domain
    bool allInside = areAllMatching(values, [&](const Vector& v) { return domain.isInside(v); });
    REQUIRE(allInside);

    // if we split the cube to octants, each of them will have approximately the same number of particles.
    StaticArray<Size, 8> octants;
    octants.fill(0);
    for (Vector& v : values) {
        const Vector idx = v + Vector(4._f);
        const Size octantIdx =
            4 * clamp(int(idx[X]), 0, 1) + 2._f * clamp(int(idx[Y]), 0, 1) + clamp(int(idx[Z]), 0, 1);
        octants[octantIdx]++;
    }
    const Size n = values.size();
    for (Size o : octants) {
        REQUIRE(o >= n / 8 - 25);
        REQUIRE(o <= n / 8 + 25);
    }
}

TEST_CASE("HexaPacking common", "[initconds]") {
    HexagonalPacking packing(EMPTY_FLAGS);
    testDistribution(&packing);
}

TEST_CASE("HexaPacking grid", "[initconds]") {
    // test that within 1.5h of each particle, there are 12 neighbours in the same distance.
    HexagonalPacking packing(EMPTY_FLAGS);
    SphericalDomain domain(Vector(0._f), 2._f);
    Array<Vector> r = packing.generate(1000, domain);
    std::unique_ptr<Abstract::Finder> finder = Factory::getFinder(GlobalSettings::getDefaults());
    finder->build(r);
    Array<NeighbourRecord> neighs;
    auto test = [&](const Size i) {
        if (getLength(r[i]) > 1.3_f) {
            // skip particles close to boundary, they don't necessarily have 12 neighbours
            return SUCCESS;
        }
        finder->findNeighbours(i, 1.5_f * r[i][H], neighs, EMPTY_FLAGS);
        if (neighs.size() != 13) { // 12 + i-th particle itself
            return makeFailed("Invalid number of neighbours: \n", neighs.size(), " == 13");
        }
        const Float expectedDist = r[i][H]; // note that dist does not have to be exactly h, only approximately
        for (auto& n : neighs) {
            if (n.index == i) {
                continue;
            }
            const Float dist = getLength(r[i] - r[n.index]);
            if (dist != approx(expectedDist, 0.1_f)) {
                return makeFailed("Invalid distance to neighbours: \n", dist, " == ", expectedDist);
            }
        }
        return SUCCESS;
    };
    REQUIRE_SEQUENCE(test, 0, r.size());
}

TEST_CASE("HexaPacking sorted", "[initconds]") {
    HexagonalPacking sorted(HexagonalPacking::Options::SORTED);
    HexagonalPacking unsorted(EMPTY_FLAGS);

    BlockDomain domain(Vector(-3._f), Vector(2._f));
    Array<Vector> r_sort = sorted.generate(1000, domain);
    Array<Vector> r_unsort = unsorted.generate(1000, domain);
    ASSERT(r_sort.size() == r_unsort.size());


    std::unique_ptr<Abstract::Finder> finder_sort = Factory::getFinder(GlobalSettings::getDefaults());
    finder_sort->build(r_sort);
    std::unique_ptr<Abstract::Finder> finder_unsort = Factory::getFinder(GlobalSettings::getDefaults());
    finder_unsort->build(r_unsort);

    // find maximum distance of neighbouring particles in memory
    Size neighCnt_sort = 0;
    Size neighCnt_unsort = 0;
    Array<Size> dists_sort;
    Array<Size> dists_unsort;
    Array<NeighbourRecord> neighs;
    for (Size i = 0; i < r_sort.size(); ++i) {
        neighCnt_sort += finder_sort->findNeighbours(i, 2._f * r_sort[i][H], neighs);
        for (auto& n : neighs) {
            const Size dist = Size(abs(int(n.index) - int(i)));
            dists_sort.push(dist);
        }
    }
    for (Size i = 0; i < r_unsort.size(); ++i) {
        neighCnt_unsort += finder_unsort->findNeighbours(i, 2._f * r_unsort[i][H], neighs);
        for (auto& n : neighs) {
            const Size dist = Size(abs(int(n.index) - int(i)));
            dists_unsort.push(dist);
        }
    }

    // sanity check
    REQUIRE(neighCnt_sort == neighCnt_unsort);

    ArrayStats<Size> stats_sort(dists_sort);
    ArrayStats<Size> stats_unsort(dists_unsort);
    REQUIRE(stats_sort.median() < stats_unsort.median());
}

TEST_CASE("CubicPacking", "[initconds]") {
    CubicPacking packing;
    testDistribution(&packing);
}

TEST_CASE("RandomDistribution", "[initconds]") {
    RandomDistribution random;
    testDistribution(&random);
    // 100 points inside block [0,1]^d, approx. distance is 100^(-1/d)
}

TEST_CASE("DiehlEtAlDistribution", "[initconds]") {
    // Diehl et al. (2012) algorithm, using uniform particle density
    DiehlEtAlDistribution diehl([](const Vector&) { return 1._f; });
    testDistribution(&diehl);
}

TEST_CASE("LinearDistribution", "[initconds]") {
    LinearDistribution linear;
    SphericalDomain domain(Vector(0.5_f), 0.5_f);
    Array<Vector> values = linear.generate(101, domain);
    REQUIRE(values.size() == 101);    
    auto test = [&](const Size i) {
        if (values[i] != approx(Vector(i / 100._f, 0._f, 0._f), 1.e-5_f)) {
            return makeFailed(values[i], " == ", Vector(i / 100._f, 0._f, 0._f));
        }
        return SUCCESS;
    };
    REQUIRE_SEQUENCE(test, 0, 100);
}
