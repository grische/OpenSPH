#pragma once

/// \file Camera.h
/// \brief Defines projection transforming 3D particles onto 2D screen
/// \author Pavel Sevecek (sevecek at sirrah.troja.mf.cuni.cz)
/// \date 2016-2017

#include "objects/geometry/Tensor.h"
#include "gui/objects/Point.h"
#include "objects/wrappers/Optional.h"

NAMESPACE_SPH_BEGIN

/// Represents a particle projected onto image plane
struct ProjectedPoint {

    /// Point in image coordinates
    Point point;

    /// Projected radius of the particle
    float radius;
};

/// Ray given by origin and target point
struct Ray {
    Vector origin;
    Vector target;
};

namespace Abstract {

    /// \brief Interface defining a camera or view, used by a renderer.
    class Camera : public Polymorphic {
    public:
        /// Returns projected position of particle on the image. If the particle is outside of the image
        /// region or is clipped by the projection, returns NOTHING.
        virtual Optional<ProjectedPoint> project(const Vector& r) const = 0;

        /// Returns a ray in particle coordinates corresponding to given point in the image plane.
        virtual Ray unproject(const Point point) const = 0;

        /// Zooms the camera. This shall be equivalent to transforming the view with scaling matrix, alhough
        /// it can be implemented differently.
        /// \param magnitude Relative zoom amount, value <1 means zooming out, value >1 means zooming in.
        virtual void zoom(const float magnitude) = 0;

        /// Transforms the current view by given matrix.
        virtual void transform(const Tensor& matrix) = 0;

        /// Moves the camera by relative offset
        virtual void pan(const Point offset) = 0;
    };
}


struct OrthoCameraData {
    /// Field of view (zoom)
    float fov;

    /// Cutoff of particles far away from origin plane
    float cutoff;

    /// Vectors defining camera plane
    Vector u, v;
};

class OrthoCamera : public Abstract::Camera {
private:
    Point imageSize;
    Point center;

    OrthoCameraData data;

    struct {
        Vector w;
    } cached;

public:
    OrthoCamera(const Point imageSize, const Point center, OrthoCameraData data)
        : imageSize(imageSize)
        , center(center)
        , data(data) {
        cached.w = cross(data.u, data.v);
    }

    virtual Optional<ProjectedPoint> project(const Vector& r) const override {
        if (data.cutoff == 0._f || abs(dot(cached.w, r)) < data.cutoff) {
            const Size x = center.x + dot(r, data.u) * data.fov;
            const Size y = center.y + dot(r, data.v) * data.fov;
            const Point point(x, imageSize.y - y - 1);
            return { { point, max(data.fov * float(r[H]), 1.f) } };
        } else {
            return NOTHING;
        }
    }

    virtual Ray unproject(const Point point) const override {
        const float x = (point.x - center.x) / data.fov;
        const float y = ((imageSize.y - point.y - 1) - center.y) / data.fov;
        Ray ray;
        ray.origin = data.u * x + data.v * y;
        ray.target = ray.origin + cached.w;
        return ray;
    }

    virtual void zoom(const float magnitude) override {
        ASSERT(magnitude > 0.f);
        data.fov *= magnitude;
    }

    virtual void transform(const Tensor& matrix) override {
        data.u = matrix * data.u;
        data.v = matrix * data.v;
        cached.w = matrix * cached.w;
    }

    virtual void pan(const Point offset) override {
        center += offset;
    }
};

NAMESPACE_SPH_END
