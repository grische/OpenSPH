#include "physics/Units.h"
#include "objects/containers/FlatSet.h"

NAMESPACE_SPH_BEGIN

UnitSystem CODE_UNITS = UnitSystem::SI();

struct UnitDesc {
    String label;
    Unit unit;

    bool operator<(const UnitDesc& other) const {
        return unit < other.unit;
    }
};

FlatMap<BasicDimension, FlatSet<UnitDesc>> UNITS = [] {
    FlatSet<UnitDesc> length(ELEMENTS_UNIQUE,
        {
            { "mm", 1._mm },
            { "cm", 1._cm },
            { "m", 1._m },
            { "km", 1._km },
            { "au", Unit::meter(Constants::au) },
        });
    FlatSet<UnitDesc> mass(ELEMENTS_UNIQUE,
        {
            { "g", 1.e-3_kg },
            { "kg", 1._kg },
            { "M_sun", Unit::kilogram(Constants::M_sun) },
            { "M_earth", Unit::kilogram(Constants::M_earth) },
        });
    FlatSet<UnitDesc> time(ELEMENTS_UNIQUE,
        {
            { "s", 1._s },
            { "min", 60._s },
            { "h", 3600._s },
            { "d", 86400._s },
            { "y", 31556926._s },
        });
    FlatSet<UnitDesc> angle(ELEMENTS_UNIQUE,
        {
            { "rad", 1._rad },
        });

    FlatMap<BasicDimension, FlatSet<UnitDesc>> units;
    units.insert(BasicDimension::LENGTH, std::move(length));
    units.insert(BasicDimension::MASS, std::move(mass));
    units.insert(BasicDimension::TIME, std::move(time));
    units.insert(BasicDimension::ANGLE, std::move(angle));
    return units;
}();

std::ostream& operator<<(std::ostream& stream, const Unit& u) {
    if (u.dimension() == UnitDimensions::dimensionless()) {
        stream << u.value(UnitSystem::SI()); // unit system irrelevant
    } else {
        BasicDimension dim = BasicDimension(-1);
        for (Size i = 0; i < DIMENSION_CNT; ++i) {
            const int power = u.dimension()[BasicDimension(i)];
            if (power != 0) {
                dim = BasicDimension(i);
                break;
            }
        }
        SPH_ASSERT(dim != BasicDimension(-1));

        UnitSystem system = CODE_UNITS;
        Optional<UnitDesc> selectedDesc;
        for (UnitDesc& desc : UNITS[dim]) {
            if (!selectedDesc) {
                selectedDesc = desc;
            } else {
                system[dim] = desc.unit.value(CODE_UNITS);
                if (u.value(system) >= 1._f) {
                    selectedDesc = desc;
                }
            }
        }
        system[dim] = selectedDesc->unit.value(CODE_UNITS);
        stream << u.value(system) << selectedDesc->label;
        if (u.dimension()[dim] != 1) {
            stream << "^" << u.dimension()[dim];
        }
    }

    return stream;
}

Expected<Unit> parseUnit(const String& text) {
    Unit u = Unit::dimensionless(1._f);
    Array<String> parts = split(text, ' ');
    for (String& part : parts) {
        if (part.empty()) {
            // multiple spaces or empty input string; allow and continue
            continue;
        }
        Array<String> valueAndPower = split(part, '^');
        if (valueAndPower.size() > 2) {
            return makeUnexpected<Unit>("More than one exponent");
        }
        SPH_ASSERT(valueAndPower.size() == 1 || valueAndPower.size() == 2);
        int power;
        if (valueAndPower.size() == 1) {
            // just unit without any power
            power = 1;
            (void)power;
        } else {
            if (Optional<int> optPower = fromString<int>(valueAndPower[1])) {
                power = optPower.value();
            } else {
                return makeUnexpected<Unit>("Cannot convert power to int");
            }
        }
    }
    return u;
}

NAMESPACE_SPH_END
