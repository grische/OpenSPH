#pragma once

#include "run/Job.h"

NAMESPACE_SPH_BEGIN

class SphereJob : public IGeometryJob {
private:
    Float radius = 1.e5_f;

public:
    explicit SphereJob(const String& name);

    virtual String className() const override;

    virtual UnorderedMap<String, ExtJobType> getSlots() const override;

    virtual VirtualSettings getSettings() override;

    virtual void evaluate(const RunSettings& global, IRunCallbacks& callbacks) override;
};

class BlockJob : public IGeometryJob {
private:
    Vector center = Vector(0._f);
    Vector dimensions = Vector(1.e5_f);

public:
    explicit BlockJob(const String& name)
        : IGeometryJob(name) {}

    virtual String className() const override {
        return "block";
    }

    virtual UnorderedMap<String, ExtJobType> getSlots() const override {
        return {};
    }

    virtual VirtualSettings getSettings() override;

    virtual void evaluate(const RunSettings& global, IRunCallbacks& UNUSED(callbacks)) override;
};

class EllipsoidJob : public IGeometryJob {
private:
    Vector semiaxes = Vector(2.e5_f, 1.e5_f, 1.e5_f);

public:
    explicit EllipsoidJob(const String& name)
        : IGeometryJob(name) {}

    virtual String className() const override {
        return "ellipsoid";
    }

    virtual UnorderedMap<String, ExtJobType> getSlots() const override {
        return {};
    }

    virtual VirtualSettings getSettings() override;

    virtual void evaluate(const RunSettings& global, IRunCallbacks& UNUSED(callbacks)) override;
};

class CylinderJob : public IGeometryJob {
private:
    Float radius = 1.e5_f;
    Float height = 2.e5_f;

public:
    explicit CylinderJob(const String& name);

    virtual String className() const override;

    virtual UnorderedMap<String, ExtJobType> getSlots() const override;

    virtual VirtualSettings getSettings() override;

    virtual void evaluate(const RunSettings& global, IRunCallbacks& UNUSED(callbacks)) override;
};

class ToroidJob : public IGeometryJob {
private:
    Float a = 1.e5_f;
    Float b = 2.5e4_f;

public:
    explicit ToroidJob(const String& name)
        : IGeometryJob(name) {}

    virtual String className() const override {
        return "toroid";
    }

    virtual UnorderedMap<String, ExtJobType> getSlots() const override {
        return {};
    }

    virtual VirtualSettings getSettings() override;

    virtual void evaluate(const RunSettings& global, IRunCallbacks& UNUSED(callbacks)) override;
};


class MaclaurinSpheroidJob : public IGeometryJob {
private:
    Float semimajorAxis = 1.e5_f;
    Float spinRate = 0._f;
    Float density = 2700._f;

public:
    explicit MaclaurinSpheroidJob(const String& name)
        : IGeometryJob(name) {}

    virtual String className() const override {
        return "Maclaurin spheroid";
    }

    virtual UnorderedMap<String, ExtJobType> getSlots() const override {
        return {};
    }

    virtual VirtualSettings getSettings() override;

    virtual void evaluate(const RunSettings& global, IRunCallbacks& UNUSED(callbacks)) override;
};

class HalfSpaceJob : public IGeometryJob {
public:
    explicit HalfSpaceJob(const String& name)
        : IGeometryJob(name) {}

    virtual String className() const override {
        return "half space";
    }

    virtual UnorderedMap<String, ExtJobType> getSlots() const override {
        return {};
    }

    virtual VirtualSettings getSettings() override;

    virtual void evaluate(const RunSettings& global, IRunCallbacks& UNUSED(callbacks)) override;
};

class GaussianSphereJob : public IGeometryJob {
private:
    Float radius = 1.e5_f;
    Float beta = 0.2_f;
    int seed = 1337;

public:
    explicit GaussianSphereJob(const String& name)
        : IGeometryJob(name) {}

    virtual String className() const override {
        return "Gaussian sphere";
    }

    virtual UnorderedMap<String, ExtJobType> getSlots() const override {
        return {};
    }

    virtual VirtualSettings getSettings() override;

    virtual void evaluate(const RunSettings& global, IRunCallbacks& UNUSED(callbacks)) override;
};


class MeshGeometryJob : public IGeometryJob {
private:
    Path path = Path("file.ply");
    Float scale = 1._f;
    bool precompute = false;

public:
    explicit MeshGeometryJob(const String& name);

    virtual String className() const override;

    virtual UnorderedMap<String, ExtJobType> getSlots() const override;

    virtual VirtualSettings getSettings() override;

    virtual void evaluate(const RunSettings& global, IRunCallbacks& callbacks) override;
};

class ParticleGeometryJob : public IGeometryJob {
private:
    Float resolution = 1.e3_f;
    Float surfaceLevel = 0.15_f;
    Float smoothingMult = 1._f;

public:
    explicit ParticleGeometryJob(const String& name)
        : IGeometryJob(name) {}

    virtual String className() const override {
        return "particle geometry";
    }

    virtual UnorderedMap<String, ExtJobType> getSlots() const override {
        return { { "particles", JobType::PARTICLES } };
    }

    virtual VirtualSettings getSettings() override;

    virtual void evaluate(const RunSettings& global, IRunCallbacks& callbacks) override;
};

class SpheresGeometryJob : public IGeometryJob {
public:
    explicit SpheresGeometryJob(const String& name)
        : IGeometryJob(name) {}

    virtual String className() const override {
        return "spheres geometry";
    }

    virtual UnorderedMap<String, ExtJobType> getSlots() const override {
        return { { "spheres", JobType::PARTICLES } };
    }

    virtual VirtualSettings getSettings() override;

    virtual void evaluate(const RunSettings& global, IRunCallbacks& callbacks) override;
};

class InvertGeometryJob : public IGeometryJob {
public:
    explicit InvertGeometryJob(const String& name)
        : IGeometryJob(name) {}

    virtual String className() const override {
        return "invert geometry";
    }

    virtual UnorderedMap<String, ExtJobType> getSlots() const override {
        return { { "geometry", JobType::GEOMETRY } };
    }

    virtual VirtualSettings getSettings() override;

    virtual void evaluate(const RunSettings& global, IRunCallbacks& callbacks) override;
};

class TransformGeometryJob : public IGeometryJob {
private:
    Vector scaling = Vector(1._f);
    Vector offset = Vector(0._f);

public:
    explicit TransformGeometryJob(const String& name)
        : IGeometryJob(name) {}

    virtual String className() const override {
        return "transform geometry";
    }

    virtual UnorderedMap<String, ExtJobType> getSlots() const override {
        return { { "geometry", JobType::GEOMETRY } };
    }

    virtual VirtualSettings getSettings() override;

    virtual void evaluate(const RunSettings& global, IRunCallbacks& callbacks) override;
};

enum class BooleanEnum {
    SET_UNION,
    SET_DIFFERENCE,
    SET_INTERSECTION,
};
static RegisterEnum<BooleanEnum> sBoolean({
    { BooleanEnum::SET_UNION, "union", "union" },
    { BooleanEnum::SET_INTERSECTION, "intersection", "intersection" },
    { BooleanEnum::SET_DIFFERENCE, "difference", "difference" },
});


class BooleanGeometryJob : public IGeometryJob {
    EnumWrapper mode;
    Vector offset = Vector(0._f);

public:
    BooleanGeometryJob(const String& name)
        : IGeometryJob(name) {
        mode = EnumWrapper(BooleanEnum::SET_DIFFERENCE);
    }

    virtual String className() const override {
        return "boolean";
    }

    virtual UnorderedMap<String, ExtJobType> getSlots() const override {
        return { { "operand A", JobType::GEOMETRY }, { "operand B", JobType::GEOMETRY } };
    }

    virtual VirtualSettings getSettings() override;

    virtual void evaluate(const RunSettings& global, IRunCallbacks& callbacks) override;
};

NAMESPACE_SPH_END
