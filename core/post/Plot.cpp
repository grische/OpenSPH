#include "post/Plot.h"
#include "io/Logger.h"
#include "objects/containers/FlatSet.h"
#include "post/Point.h"
#include "quantities/IMaterial.h"
#include "quantities/Storage.h"
#include "system/Statistics.h"
#include <set>

NAMESPACE_SPH_BEGIN

// ----------------------------------------------------------------------------------------------------------
// SpatialPlot
// ----------------------------------------------------------------------------------------------------------

/// \note SpatialPlot should be only used through derived classes, so it's ok to put templated functions here
template <typename TDerived>
void SpatialPlot<TDerived>::onTimeStep(const Storage& storage, const Statistics& UNUSED(stats)) {
    // no temporal dependence - reset everything
    this->clear();

    Array<PlotPoint> particlePoints;
    ArrayView<const Float> quantity = storage.getValue<Float>(id);
    ArrayView<const Vector> r = storage.getValue<Vector>(QuantityId::POSITION);
    for (Size i = 0; i < r.size(); ++i) {
        PlotPoint p{ getX(r[i]), quantity[i] };
        particlePoints.push(p);
        ranges.x.extend(p.x);
        ranges.y.extend(p.y);
    }
    std::sort(particlePoints.begin(), particlePoints.end(), [](PlotPoint& p1, PlotPoint& p2) {
        return p1.x < p2.x;
    });

    if (!binCnt) {
        points = std::move(particlePoints);
    } else {
        SPH_ASSERT(binCnt.value() >= 1);
        points.resize(binCnt.value());
        Array<Size> weights(binCnt.value());
        points.fill(PlotPoint(0.f, 0.f));
        weights.fill(0);

        const Float lastX = particlePoints[particlePoints.size() - 1].x;
        for (PlotPoint& p : particlePoints) {
            const Size bin = min(Size(p.x * (binCnt.value() - 1) / lastX), binCnt.value() - 1);
            points[bin] += p;
            weights[bin]++;
        }

        for (Size i = 0; i < points.size(); ++i) {
            if (weights[i] > 0) {
                points[i].x /= weights[i];
                points[i].y /= weights[i];
            } else {
                SPH_ASSERT(points[i] == PlotPoint(0.f, 0.f));
            }
        }
    }
}

template <typename TDerived>
void SpatialPlot<TDerived>::clear() {
    points.clear();
    ranges.x = ranges.y = Interval();
}

template <typename TDerived>
void SpatialPlot<TDerived>::plot(IDrawingContext& dc) const {
    for (Size i = 0; i < points.size(); ++i) {
        if (i > 0) {
            dc.drawLine(points[i], points[i - 1]);
        }
        dc.drawPoint(points[i]);
    }
}

RadialDistributionPlot::RadialDistributionPlot(const QuantityId id, const Optional<Size> binCnt)
    : SpatialPlot<RadialDistributionPlot>(id, binCnt) {}


// ----------------------------------------------------------------------------------------------------------
// TemporalPlot
// ----------------------------------------------------------------------------------------------------------

void TemporalPlot::onTimeStep(const Storage& storage, const Statistics& stats) {
    // add new point to the queue
    const Float t = stats.get<Float>(StatisticsId::RUN_TIME);
    if (t - lastTime < actPeriod) {
        return;
    }
    lastTime = t;

    const Float y = integral.evaluate(storage);
    points.pushBack(PlotPoint{ t, y });

    if (params.segment == INFTY && points.size() > params.maxPointCnt) {
        // plot is unnecessarily detailed, drop every second point to reduce the drawing time
        Queue<PlotPoint> newPoints;
        for (Size i = 0; i < points.size(); i += 2) {
            newPoints.pushBack(points[i]);
        }
        points = std::move(newPoints);
        // also add new points with double period
        actPeriod *= 2._f;
    }

    // pop expired points
    bool needUpdateRange = false;
    while (!points.empty() && this->isExpired(points.front().x, t)) {
        points.popFront();
        needUpdateRange = true;
    }

    // update ranges
    if (needUpdateRange && params.shrinkY) {
        // we removed some points, so we don't know how much to shrink, let's construct it from scrach
        ranges.y = Interval();
        for (PlotPoint& p : points) {
            ranges.y.extend(p.y);
        }
    } else if (!points.empty()) {
        // we just added points, no need to shrink the range, just extend it with the new point
        ranges.y.extend(points.back().y);
    }
    // make sure the y-range is larger than the minimal allowed value
    if (ranges.y.size() < params.minRangeY) {
        const Float dy = 0.5_f * (params.minRangeY - ranges.y.size());
        SPH_ASSERT(dy >= 0._f, params.minRangeY, ranges.y);
        ranges.y.extend(ranges.y.upper() + dy);
        ranges.y.extend(ranges.y.lower() - dy);
    }
    if (points.empty()) {
        ranges.x = Interval(); // nothing to draw
    } else if (params.fixedRangeX.empty()) {
        const Float t0 = max(points.front().x, t - params.segment);
        ranges.x = Interval(t0, t);
    } else {
        ranges.x = params.fixedRangeX;
    }
}

void TemporalPlot::clear() {
    points.clear();
    lastTime = -INFTY;
    ranges.x = ranges.y = Interval();
    actPeriod = params.period;
}

void TemporalPlot::plot(IDrawingContext& dc) const {
    if (points.empty()) {
        return;
    }
    AutoPtr<IDrawPath> path = dc.drawPath();
    for (const PlotPoint& p : points) {
        dc.drawPoint(p);
        path->addPoint(p);
    }
    path->endPath();
}

bool TemporalPlot::isExpired(const Float x, const Float t) const {
    if (params.fixedRangeX.empty()) {
        // compare with the segment
        return x < t - params.segment;
    } else {
        // compare with the range
        return !params.fixedRangeX.contains(t);
    }
}

// ----------------------------------------------------------------------------------------------------------
// HistogramPlot
// ----------------------------------------------------------------------------------------------------------

void HistogramPlot::onTimeStep(const Storage& storage, const Statistics& stats) {
    const Float time = stats.get<Float>(StatisticsId::RUN_TIME);
    if (time - lastTime < period) {
        return;
    }
    lastTime = time;

    Post::HistogramParams params;
    params.binCnt = 0;
    if (interval) {
        params.range = interval.value();
    }
    points = Post::getDifferentialHistogram(storage, id, Post::HistogramSource::PARTICLES, params);

    this->clear();
    for (Post::HistPoint& p : points) {
        ranges.x.extend(p.value);
        ranges.y.extend(p.count);
    }
    // always include y=0
    ranges.y.extend(0);
}

void HistogramPlot::clear() {
    ranges.x = ranges.y = Interval();
}

void HistogramPlot::plot(IDrawingContext& dc) const {
    if (points.empty()) {
        return;
    }
    for (Size i = 0; i < points.size() - 1; ++i) {
        dc.drawLine(
            { points[i].value, Float(points[i].count) }, { points[i + 1].value, Float(points[i].count) });
        dc.drawLine({ points[i + 1].value, Float(points[i].count) },
            { points[i + 1].value, Float(points[i + 1].count) });
    }
}

// ----------------------------------------------------------------------------------------------------------
// AngularHistogramPlot
// ----------------------------------------------------------------------------------------------------------

AngularHistogramPlot::AngularHistogramPlot(const Float period)
    : period(period) {}

void AngularHistogramPlot::onTimeStep(const Storage& storage, const Statistics& stats) {
    const Float time = stats.get<Float>(StatisticsId::RUN_TIME);
    if (time - lastTime < period) {
        return;
    }
    lastTime = time;

    ArrayView<const Vector> v = storage.getDt<Vector>(QuantityId::POSITION);
    Array<Float> angles(v.size());
    for (Size i = 0; i < v.size(); ++i) {
        angles[i] = atan2(v[i][Y], v[i][X]);
    }

    Post::HistogramParams params;
    params.binCnt = 0;
    params.range = Interval(-PI, PI);
    Array<Post::HistPoint> hist = Post::getDifferentialHistogram(angles, params);

    this->clear();
    points.clear();
    for (const Post::HistPoint& p : hist) {
        const Float x = p.count * cos(p.value);
        const Float y = p.count * sin(p.value);
        points.push(PlotPoint{ x, y });
        ranges.x.extend(x);
        ranges.y.extend(y);
    }

    // include (0,0)
    ranges.x.extend(0._f);
    ranges.y.extend(0._f);

    // make square
    ranges.x.extend(ranges.y);
    ranges.y.extend(ranges.x);
}

void AngularHistogramPlot::clear() {
    ranges.x = Interval();
    ranges.y = Interval();
}

void AngularHistogramPlot::plot(IDrawingContext& dc) const {
    if (points.size() <= 1) {
        return;
    }
    for (Size i = 0; i < points.size() - 1; ++i) {
        dc.drawLine(points[i], points[i + 1]);
    }
    dc.drawLine(points.back(), points.front());
}

// ----------------------------------------------------------------------------------------------------------
// SfdPlot
// ----------------------------------------------------------------------------------------------------------

SfdPlot::SfdPlot(const Flags<Post::ComponentFlag> connectivity, const Float period)
    : period(period) {
    source = Post::HistogramSource::COMPONENTS;
    connect = connectivity;

    if (connect.has(Post::ComponentFlag::ESCAPE_VELOCITY)) {
        name = "Predicted SFD";
    } else {
        name = "Current SFD";
    }
}

SfdPlot::SfdPlot(const Float period)
    : period(period) {
    source = Post::HistogramSource::PARTICLES;
    name = "Particle SFD";
}

String SfdPlot::getCaption() const {
    return name;
}

void SfdPlot::onTimeStep(const Storage& storage, const Statistics& stats) {
    const Float time = stats.get<Float>(StatisticsId::RUN_TIME);
    if (time - lastTime < period) {
        return;
    }
    lastTime = time;
    Post::HistogramParams params;
    params.components.flags = connect;
    params.velocityCutoff = 3.e3_f; // km/s  /// \todo generalize
    if (storage.getMaterialCnt() > 0) {
        const BodySettings& body = storage.getMaterial(0)->getParams();
        if (body.has(BodySettingsId::DENSITY)) {
            params.referenceDensity = body.get<Float>(BodySettingsId::DENSITY);
        }
    }
    Array<Post::HistPoint> points =
        Post::getCumulativeHistogram(storage, Post::HistogramId::EQUIVALENT_MASS_RADII, source, params);

    ranges.x = ranges.y = Interval();
    sfd.clear();
    sfd.reserve(points.size());
    for (Post::HistPoint& p : points) {
        SPH_ASSERT(p.value > 0._f && p.count > 0);
        const Float value = log10(p.value);
        const Float count = log10(Float(p.count));
        ranges.x.extend(value);
        ranges.y.extend(count);
        sfd.emplaceBack(PlotPoint{ value, count });
    }
}

void SfdPlot::clear() {
    ranges.x = ranges.y = Interval();
    lastTime = 0._f;
    sfd.clear();
}

void SfdPlot::plot(IDrawingContext& dc) const {
    if (!sfd.empty()) {
        AutoPtr<IDrawPath> path = dc.drawPath();
        for (Size i = 0; i < sfd.size() - 1; ++i) {
            dc.drawPoint(sfd[i]);
            path->addPoint(sfd[i]);
        }
        dc.drawPoint(sfd.back());
        path->addPoint(sfd.back());
        path->endPath();
    }
}

// ----------------------------------------------------------------------------------------------------------
// DataPlot
// ----------------------------------------------------------------------------------------------------------

DataPlot::DataPlot(const Array<Post::HistPoint>& points, const Flags<AxisScaleEnum> scale, const String& name)
    : name(name) {
    for (const Post::HistPoint& p : points) {
        if (scale.has(AxisScaleEnum::LOG_X) && p.value <= 0._f) {
            continue;
        }
        if (scale.has(AxisScaleEnum::LOG_Y) && p.count <= 0) {
            continue;
        }
        const Float value = scale.has(AxisScaleEnum::LOG_X) ? log10(p.value) : p.value;
        const Float count = scale.has(AxisScaleEnum::LOG_Y) ? log10(Float(p.count)) : Float(p.count);
        ranges.x.extend(value);
        ranges.y.extend(count);
        values.emplaceBack(PlotPoint{ value, count });
    }
}

String DataPlot::getCaption() const {
    return name;
}

void DataPlot::onTimeStep(const Storage& UNUSED(storage), const Statistics& UNUSED(stats)) {
    // plot is contant
}

void DataPlot::clear() {
    // data are fixed, we cannot clear anything
}

void DataPlot::plot(IDrawingContext& dc) const {
    if (!values.empty()) {
        AutoPtr<IDrawPath> path = dc.drawPath();
        for (Size i = 0; i < values.size() - 1; ++i) {
            dc.drawPoint(values[i]);
            path->addPoint(values[i]);
        }
        dc.drawPoint(values.back());
        path->addPoint(values.back());
        path->endPath();
    }
}

// ----------------------------------------------------------------------------------------------------------
// MultiPlot
// ----------------------------------------------------------------------------------------------------------

void MultiPlot::onTimeStep(const Storage& storage, const Statistics& stats) {
    ranges.x = ranges.y = Interval();
    for (auto& plot : plots) {
        plot->onTimeStep(storage, stats);
        ranges.x.extend(plot->rangeX());
        ranges.y.extend(plot->rangeY());
    }
}

void MultiPlot::clear() {
    ranges.x = ranges.y = Interval();
    for (auto& plot : plots) {
        plot->clear();
    }
}

void MultiPlot::plot(IDrawingContext& dc) const {
    for (auto plotAndIndex : iterateWithIndex(plots)) {
        dc.setStyle(plotAndIndex.index());
        plotAndIndex.value()->plot(dc);
    }
}

// ----------------------------------------------------------------------------------------------------------
// getTics
// ----------------------------------------------------------------------------------------------------------

Array<Float> getLinearTics(const Interval& interval, const Size minCount) {
    Float order = floor(log10(interval.size()));

    auto getTicsInterval = [interval](const Float step) {
        return Interval(ceil(interval.lower() / step) * step, floor(interval.upper() / step) * step);
    };

    Float step;
    while (true) {
        step = pow(10._f, order);
        SPH_ASSERT(step >= std::numeric_limits<Float>::denorm_min());
        if (getTicsInterval(step).size() < step * minCount) {
            order--;
        } else {
            break;
        }
    }

    // Now we have step 10^order, which might be too small, we thus also allow step 2*10^order (2, 4, 6, ...)
    // and 5*10^order (5, 10, 15, ...). This can be modified if necessary.
    if (getTicsInterval(5 * step).size() >= 5 * step * minCount) {
        step *= 5;
    } else if (getTicsInterval(2 * step).size() >= 2 * step * minCount) {
        step *= 2;
    }
    const Interval ticsInterval = getTicsInterval(step);

    Array<Float> tics;
    for (Float x = ticsInterval.lower(); x <= ticsInterval.upper() + EPS * step; x += step) {
        tics.push(x);
    }
    SPH_ASSERT(tics.size() >= minCount && tics.size() < 10 * minCount);
    return tics;
}

Array<Float> getLogTics(const Interval& interval, const Size minCount, const Size maxCount) {
    SPH_ASSERT(interval.lower() > EPS); // could be relaxed if we need to plot some very small quantities
    const Float fromOrder = floor(log10(interval.lower()));
    const Float toOrder = ceil(log10(interval.upper()));
    SPH_ASSERT(isReal(fromOrder) && isReal(toOrder) && toOrder >= fromOrder);

    FlatSet<Float> tics;
    auto tryAdd = [&](const Float value) {
        if (interval.contains(value)) {
            tics.insert(value);
        }
    };

    // try stepping in integer orders (1, 10, 100, ...)
    for (Float order = fromOrder; order <= toOrder; order++) {
        const Float value = pow(10._f, order);
        tryAdd(value);
    }

    if (tics.size() < minCount) {
        // add 2, 5, 20, 50, ...
        for (Float order = fromOrder; order <= toOrder; order++) {
            const Float value = pow(10._f, order);
            tryAdd(2 * value);
            tryAdd(5 * value);
        }
    }
    if (tics.size() < minCount) {
        // add more values
        for (Float order = fromOrder; order <= toOrder; order++) {
            const Float value = pow(10._f, order);
            tryAdd(1.5_f * value);
            tryAdd(2.5_f * value);
            tryAdd(7.5_f * value);
        }
    }
    if (tics.size() > maxCount) {
        Size removeEach = (tics.size() + maxCount - 1) / maxCount;
        Size i = 0;
        for (auto iter = tics.begin(); iter != tics.end(); ++i) {
            if (i % removeEach == 1) {
                iter = tics.erase(iter);
            } else {
                iter++;
            }
        }
    }

    Array<Float> result;
    for (Float t : tics) {
        result.push(t);
    }
    return result;
}

Array<Float> getHybridTics(const Interval& interval, const Size minCount) {
    SPH_ASSERT(interval.lower() < -1.f && interval.upper() > 1.f, interval);
    Array<Float> negLogTics = getLogTics(Interval(1.f, -interval.lower()), minCount / 2, minCount);
    Array<Float> linearTics = getLinearTics(Interval(-1.f, 1.f), 3);
    Array<Float> posLogTics = getLogTics(Interval(1.f, interval.upper()), minCount / 2, minCount);

    for (Float& tic : negLogTics) {
        tic *= -1;
    }
    FlatSet<Float> result;
    result.insert(negLogTics.begin(), negLogTics.end());
    result.insert(linearTics.begin(), linearTics.end());
    result.insert(posLogTics.begin(), posLogTics.end());
    return std::move(result).array();
}

NAMESPACE_SPH_END
