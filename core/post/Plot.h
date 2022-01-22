#pragma once

/// \file Plot.h
/// \brief Drawing quantity values as functions of time or spatial coordinates
/// \author Pavel Sevecek (sevecek at sirrah.troja.mff.cuni.cz)
/// \date 2016-2021

#include "objects/containers/Queue.h"
#include "objects/utility/OperatorTemplate.h"
#include "physics/Integrals.h"
#include "post/Analysis.h"

NAMESPACE_SPH_BEGIN

struct PlotPoint;
struct ErrorPlotPoint;
class AffineMatrix2;

class IDrawPath : public Polymorphic {
public:
    /// \brief Adds a next point on the path
    virtual void addPoint(const PlotPoint& point) = 0;

    /// \brief Closes the path, connecting to the first point on the path
    virtual void closePath() = 0;

    /// \brief Finalizes the path. Does not connect the last point to anything.
    virtual void endPath() = 0;
};

/// \brief Abstraction of a drawing context.
///
/// Object operates in plot coordinates.
class IDrawingContext : public Polymorphic {
public:
    /// \brief Adds a single point to the plot
    ///
    /// The plot is drawn by implementation-defined style.
    virtual void drawPoint(const PlotPoint& point) = 0;

    /// \brief Adds a point with error bars to the plot.
    virtual void drawErrorPoint(const ErrorPlotPoint& point) = 0;

    /// \brief Draws a line connecting two points.
    ///
    /// The ending points are not drawn; call \ref drawPoint manually if you wish to draw both lines and the
    /// points.
    virtual void drawLine(const PlotPoint& from, const PlotPoint& to) = 0;

    /// \brief Draws a path connecting points.
    ///
    /// The path copies the state from the parent drawing context, so if a drawing style of the context
    /// changes, the change does not affect already existing paths.
    virtual AutoPtr<IDrawPath> drawPath() = 0;

    /// \brief Changes the current drawing style.
    ///
    /// Interpretation of the style is implementation-defined. It is assumed that the derived classes will
    /// have a predefined set of drawing styles, this function will then selects the style with appropriate
    /// index.
    /// \param index Index of the selected style.
    virtual void setStyle(const Size index) = 0;

    /// \brief Applies the given tranformation matrix on all primitives.
    ///
    /// This does not affect already drawn primitives.
    virtual void setTransformMatrix(const AffineMatrix2& matrix) = 0;
};


/// \brief Interface for constructing generic plots from quantities stored in storage.
///
/// The plot can currently be only 2D, typically showing a quantity dependence on time or on some spatial
/// coordinate.
class IPlot : public Polymorphic {
protected:
    struct {
        Interval x;
        Interval y;
    } ranges;

public:
    /// \brief Returns the plotted range in x-coordinate.
    Interval rangeX() const {
        return ranges.x;
    }

    /// \brief Returns the plotted range in y-coordinate.
    Interval rangeY() const {
        return ranges.y;
    }

    /// \brief Returns the caption of the plot
    virtual String getCaption() const = 0;

    /// \brief Updates the plot with new data.
    ///
    /// Called every time step.
    virtual void onTimeStep(const Storage& storage, const Statistics& stats) = 0;

    /// \brief Clears all cached data, prepares for next run.
    virtual void clear() = 0;

    /// \brief Draws the plot into the drawing context
    virtual void plot(IDrawingContext& dc) const = 0;
};

/// \brief Base class for plots showing a dependence of given quantity on a spatial coordinate.
///
/// Currently only works with scalar quantities.
/// \todo Needs to limit the number of drawn points - we definitely dont want to draw 10^5 points.
template <typename TDerived>
class SpatialPlot : public IPlot {
protected:
    QuantityId id;
    Array<PlotPoint> points;
    Optional<Size> binCnt;

public:
    /// \brief Constructs the spatial plot.
    /// \param id Quantity to plot
    /// \param binCnt Number of points in the plot; if NOTHING, each particle is plotted as a point.
    explicit SpatialPlot(const QuantityId id, const Optional<Size> binCnt = NOTHING)
        : id(id)
        , binCnt(binCnt) {}

    virtual String getCaption() const override {
        return getMetadata(id).quantityName;
    }

    virtual void onTimeStep(const Storage& storage, const Statistics& UNUSED(stats)) override;

    virtual void clear() override;

    virtual void plot(IDrawingContext& dc) const override;

private:
    Float getX(const Vector r) const {
        return static_cast<const TDerived*>(this)->getX(r);
    }
};

/// \brief Plots a dependence of given quantity on the distance from given axis.
class AxialDistributionPlot : public SpatialPlot<AxialDistributionPlot> {
private:
    Vector axis;

public:
    AxialDistributionPlot(const Vector& axis, const QuantityId id, const Optional<Size> binCnt = NOTHING)
        : SpatialPlot<AxialDistributionPlot>(id, binCnt)
        , axis(axis) {}

    INLINE Float getX(const Vector& r) const {
        return getLength(r - dot(r, axis) * axis);
    }
};

/// \brief Plots a dependence of given quantity on the distance from the origin
class RadialDistributionPlot : public SpatialPlot<RadialDistributionPlot> {
public:
    RadialDistributionPlot(const QuantityId id, const Optional<Size> binCnt = NOTHING);

    INLINE Float getX(const Vector& r) const {
        return getLength(r);
    }
};

/// \brief Plot of temporal dependence of a scalar quantity.
///
/// Plot shows a given segment of history of a quantity. This segment moves as time goes. Alternatively, the
/// segment can be (formally) infinite, meaning the plot shows the whole history of a quantity; the x-range is
/// rescaled as time goes.
class TemporalPlot : public IPlot {
public:
    /// Parameters of the plot
    struct Params {
        /// Plotted time segment
        Float segment = INFTY;

        /// Fixed x-range for the plot. If empty, a dynamic range is used.
        Interval fixedRangeX = Interval{};

        /// Minimal size of the y-range
        Float minRangeY = 0._f;

        /// When discarting points out of plotted range, shrink y-axis to fit currently visible points
        bool shrinkY = false;

        /// Maximum number of points on the plot. When exceeded, every second point is removed and the plot
        /// period is doubled.
        Size maxPointCnt = 100;

        /// Time that needs to pass before a new point is added
        Float period = 0._f;
    };


private:
    /// Integral being plotted
    IntegralWrapper integral;

    /// Points on the timeline; x coordinate is time, y coordinate is the value of the quantity
    Queue<PlotPoint> points;

    /// Last time a point has been added
    Float lastTime = -INFTY;

    /// Parameters used to create the plot.
    const Params params;

    /// Current period of the plot. Initialized with params.period, but may change during the run.
    Float actPeriod;

public:
    /// Creates a plot showing the whole history of given integral.
    TemporalPlot(const IntegralWrapper& integral, const Params& params)
        : integral(integral)
        , params(params) {
        SPH_ASSERT(params.segment > 0._f);
        actPeriod = params.period;
    }

    virtual String getCaption() const override {
        return integral.getName();
    }

    virtual void onTimeStep(const Storage& storage, const Statistics& stats) override;

    virtual void clear() override;

    virtual void plot(IDrawingContext& dc) const override;

private:
    /// Checks if given point is presently expired and should be removed from the queue.
    bool isExpired(const Float x, const Float t) const;
};


/// \brief Differential histogram of quantities
///
/// Plot doesn't store any history, it is drawed each timestep independently.
class HistogramPlot : public IPlot {
protected:
    /// ID of a quantity from which the histogram is constructed.
    Post::ExtHistogramId id;

    /// Points representing the histogram
    Array<Post::HistPoint> points;

    /// Interval for which the histogram is constructed.
    ///
    /// If NOTHING, the interval is created by enclosing all x values.
    Optional<Interval> interval;

    /// Period of redrawing the histogram. Zero means the histogram is drawn every time step.
    Float period;

    /// Displayed name of the histogram.
    String name;

    Float lastTime = -INFTY;

public:
    HistogramPlot(const Post::HistogramId id,
        const Optional<Interval> interval,
        const Float period,
        const String& name)
        : id(id)
        , interval(interval)
        , period(period)
        , name(name) {}

    HistogramPlot(const QuantityId id, const Optional<Interval> interval, const Float period)
        : id(id)
        , interval(interval)
        , period(period) {
        name = getMetadata(id).quantityName;
    }

    virtual String getCaption() const override {
        return name;
    }

    virtual void onTimeStep(const Storage& storage, const Statistics& stats) override;

    virtual void clear() override;

    virtual void plot(IDrawingContext& dc) const override;
};

/// \brief Differential histogram of angular distribution.
///
/// Currently fixed for velocities.
class AngularHistogramPlot : public IPlot {
    /// Histogram points
    Array<PlotPoint> points;

    Float period;

    Float lastTime = -INFTY;

public:
    explicit AngularHistogramPlot(const Float period);

    virtual String getCaption() const override {
        return "Angular histogram of velocities";
    }

    virtual void onTimeStep(const Storage& storage, const Statistics& stats) override;

    virtual void clear() override;

    virtual void plot(IDrawingContext& dc) const override;
};

class SfdPlot : public IPlot {
private:
    Post::HistogramSource source;
    Flags<Post::ComponentFlag> connect;
    Float period;
    String name;

    Float lastTime = -INFTY;
    Array<PlotPoint> sfd;

public:
    /// \brief Plots SFD of components, given their connectivity.
    SfdPlot(const Flags<Post::ComponentFlag> connectivity, const Float period);

    /// \brief Plots SFD of individual particles, makes only sense when merging the particles on collisions.
    SfdPlot(const Float period);

    virtual String getCaption() const override;

    virtual void onTimeStep(const Storage& storage, const Statistics& stats) override;

    virtual void clear() override;

    virtual void plot(IDrawingContext& dc) const override;
};

enum class AxisScaleEnum {
    LOG_X = 1 << 0,
    LOG_Y = 1 << 1,
};

/// \brief Plots given array of points.
class DataPlot : public IPlot {
private:
    Array<PlotPoint> values;
    String name;

public:
    DataPlot(const Array<Post::HistPoint>& points, const Flags<AxisScaleEnum> scale, const String& name);

    virtual String getCaption() const override;

    virtual void onTimeStep(const Storage& storage, const Statistics& stats) override;

    virtual void clear() override;

    virtual void plot(IDrawingContext& dc) const override;
};

/// \brief Helper object used for drawing multiple plots into the same device.
class MultiPlot : public IPlot {
private:
    Array<AutoPtr<IPlot>> plots;

public:
    explicit MultiPlot(Array<AutoPtr<IPlot>>&& plots)
        : plots(std::move(plots)) {}

    virtual String getCaption() const override {
        return plots[0]->getCaption(); /// ??
    }

    virtual void onTimeStep(const Storage& storage, const Statistics& stats) override;

    virtual void clear() override;

    virtual void plot(IDrawingContext& dc) const override;
};

/// \brief Returns the tics to be drawn on a linear axis of a plot.
///
/// The tics are not necessarily equidistant.
Array<Float> getLinearTics(const Interval& interval, const Size minCount);

/// \brief Returns the tics to be drawn on a logarithmic axis of a plot.
Array<Float> getLogTics(const Interval& interval, const Size minCount, const Size maxCount);

/// \brief Returns the tics to be drawn on a hybrid axis of a plot.
Array<Float> getHybridTics(const Interval& interval, const Size minCount);

NAMESPACE_SPH_END
