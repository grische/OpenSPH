#pragma once

#include "gui/Color.h"
#include "objects/containers/Array.h"
#include "quantities/QuantityKey.h"

NAMESPACE_SPH_BEGIN

enum class PaletteScale { LINEAR, LOGARITHMIC, HYBRID };

class Palette {
private:
    struct Point {
        float value;
        Color color;
    };
    Array<Point> points;

public:
    Palette() = default;

    Palette(Array<Point>&& controlPoints)
        : points(std::move(controlPoints)) {
#ifdef DEBUG
        // sanity check, points must be sorted
        for (Size i = 0; i < points.size() - 1; ++i) {
            ASSERT(points[i].value < points[i + 1].value);
        }
#endif
    }

    Range range() const {
        ASSERT(points.size() >= 2);
        return Range(points[0].value, points[points.size() - 1].value);
    }

    Color operator()(const float value) {
        ASSERT(points.size() >= 2);
        if (value <= points[0].value) {
            return points[0].color;
        }
        if (value >= points[points.size() - 1].value) {
            return points[points.size() - 1].color;
        }
        for (Size i = 0; i < points.size() - 1; ++i) {
            if (Range(points[i].value, points[i + 1].value).contains(value)) {
                // interpolate
                const float x = (points[i + 1].value - value) / (points[i + 1].value - points[i].value);
                return points[i].color * x + points[i + 1].color * (1.f - x);
            }
        }
        STOP;
    }

    /// Default palette for given quantity
    static Palette forQuantity(const QuantityKey key, const Range range) {
        const Float x0 = Float(range.lower());
        const Float dx = Float(range.size());
        switch (key) {
        case QuantityKey::PRESSURE:
            return Palette({ { x0, Color(0.f, 0.f, 0.2f) }, { x0 + dx, Color(1.f, 0.2f, 0.2f) } });
        case QuantityKey::DENSITY:
            return Palette({ { x0, Color(0.f, 0.f, 0.2f) }, { x0 + dx, Color(1.f, 0.2f, 0.2f) } });
        case QuantityKey::POSITIONS: // velocity
            return Palette({ { x0, Color(0.0f, 0.0f, 0.2f) },
                             { x0 + 0.2f * dx, Color(0.0f, 0.0f, 1.0f) },
                             { x0 + 0.5f * dx, Color(1.0f, 0.0f, 0.2f) },
                             { x0 + dx, Color(1.0f, 1.0f, 0.2f) } });
        case QuantityKey::DAMAGE:
            return Palette({ { x0, Color(0.1f, 0.1f, 0.1f) }, { x0 + dx, Color(0.9f, 0.9f, 0.9f) } });
        default:
            NOT_IMPLEMENTED;
        }
    }
};

NAMESPACE_SPH_END
