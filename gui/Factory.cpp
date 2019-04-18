#include "gui/Factory.h"
#include "gui/Config.h"
#include "gui/objects/Camera.h"
#include "gui/objects/Colorizer.h"
#include "gui/renderers/Brdf.h"
#include "gui/renderers/MeshRenderer.h"
#include "gui/renderers/ParticleRenderer.h"
#include "gui/renderers/RayTracer.h"

NAMESPACE_SPH_BEGIN

AutoPtr<ICamera> Factory::getCamera(const GuiSettings& settings, const Pixel size) {
    CameraEnum cameraId = settings.get<CameraEnum>(GuiSettingsId::CAMERA);
    switch (cameraId) {
    case CameraEnum::ORTHO: {
        OrthoCameraData data;
        const float fov = settings.get<Float>(GuiSettingsId::ORTHO_FOV);
        if (fov != 0.f) {
            ASSERT(fov > 0.f);
            data.fov = 0.5_f * size.y / fov;
        } else {
            data.fov = NOTHING;
        }
        data.zoffset = settings.get<Float>(GuiSettingsId::ORTHO_ZOFFSET);
        data.cutoff = settings.get<Float>(GuiSettingsId::ORTHO_CUTOFF);
        if (data.cutoff.value() == 0._f) {
            data.cutoff = NOTHING;
        }
        const OrthoEnum id = settings.get<OrthoEnum>(GuiSettingsId::ORTHO_PROJECTION);
        switch (id) {
        case OrthoEnum::XY:
            data.u = Vector(1._f, 0._f, 0._f);
            data.v = Vector(0._f, 1._f, 0._f);
            break;
        case OrthoEnum::XZ:
            data.u = Vector(1._f, 0._f, 0._f);
            data.v = Vector(0._f, 0._f, 1._f);
            break;
        case OrthoEnum::YZ:
            data.u = Vector(0._f, 1._f, 0._f);
            data.v = Vector(0._f, 0._f, 1._f);
            break;
        default:
            NOT_IMPLEMENTED;
        }
        const Vector center(settings.get<Vector>(GuiSettingsId::ORTHO_VIEW_CENTER));
        return makeAuto<OrthoCamera>(size, Pixel(int(center[X]), int(center[Y])), data);
    }
    case CameraEnum::PERSPECTIVE: {
        PerspectiveCameraData data;
        data.position = settings.get<Vector>(GuiSettingsId::PERSPECTIVE_POSITION);
        data.target = settings.get<Vector>(GuiSettingsId::PERSPECTIVE_TARGET);
        data.up = settings.get<Vector>(GuiSettingsId::PERSPECTIVE_UP);
        data.fov = settings.get<Float>(GuiSettingsId::PERSPECTIVE_FOV);
        data.clipping = Interval(settings.get<Float>(GuiSettingsId::PERSPECTIVE_CLIP_NEAR),
            settings.get<Float>(GuiSettingsId::PERSPECTIVE_CLIP_FAR));
        const int trackedIndex = settings.get<int>(GuiSettingsId::PERSPECTIVE_TRACKED_PARTICLE);
        if (trackedIndex >= 0) {
            data.tracker = makeClone<ParticleTracker>(trackedIndex);
        }
        return makeAuto<PerspectiveCamera>(size, data);
    }
    default:
        NOT_IMPLEMENTED;
    }
}

AutoPtr<IRenderer> Factory::getRenderer(IScheduler& scheduler, const GuiSettings& settings) {
    RendererEnum id = settings.get<RendererEnum>(GuiSettingsId::RENDERER);
    switch (id) {
    case RendererEnum::NONE:
        class NullRenderer : public IRenderer {
            virtual void initialize(const Storage&, const IColorizer&, const ICamera&) override {}
            virtual void render(const RenderParams&, Statistics&, IRenderOutput&) const override {}
            virtual void cancelRender() override {}
        };
        return makeAuto<NullRenderer>();
    case RendererEnum::PARTICLE:
        return makeAuto<ParticleRenderer>(scheduler, settings);
    case RendererEnum::MESH:
        return makeAuto<MeshRenderer>(scheduler, settings);
    case RendererEnum::RAYTRACER:
        return makeAuto<RayTracer>(scheduler, settings);
    default:
        NOT_IMPLEMENTED;
    }
}

AutoPtr<IBrdf> Factory::getBrdf(const GuiSettings& UNUSED(settings)) {
    return makeAuto<LambertBrdf>(1._f);
}

static AutoPtr<IColorizer> getColorizer(const GuiSettings& settings, const ColorizerId id) {
    using Factory::getPalette;

    switch (id) {
    case ColorizerId::VELOCITY:
        return makeAuto<VelocityColorizer>(getPalette(id));
    case ColorizerId::ACCELERATION:
        return makeAuto<AccelerationColorizer>(getPalette(id));
    case ColorizerId::MOVEMENT_DIRECTION:
        return makeAuto<DirectionColorizer>(Vector(0._f, 0._f, 1._f), getPalette(id));
    case ColorizerId::COROTATING_VELOCITY:
        return makeAuto<CorotatingVelocityColorizer>(getPalette(ColorizerId::VELOCITY));
    case ColorizerId::DENSITY_PERTURBATION:
        return makeAuto<DensityPerturbationColorizer>(getPalette(id));
    case ColorizerId::SUMMED_DENSITY:
        return makeAuto<SummedDensityColorizer>(
            RunSettings::getDefaults(), getPalette(ColorizerId(QuantityId::DENSITY)));
    case ColorizerId::TOTAL_ENERGY:
        return makeAuto<EnergyColorizer>(getPalette(id));
    case ColorizerId::TEMPERATURE:
        return makeAuto<TemperatureColorizer>();
    case ColorizerId::TOTAL_STRESS:
        return makeAuto<StressColorizer>(getPalette(id));
    case ColorizerId::YIELD_REDUCTION:
        return makeAuto<YieldReductionColorizer>(getPalette(id));
    case ColorizerId::DAMAGE_ACTIVATION:
        return makeAuto<DamageActivationColorizer>(getPalette(id));
    case ColorizerId::RADIUS:
        return makeAuto<RadiusColorizer>(getPalette(id));
    case ColorizerId::BOUNDARY:
        return makeAuto<BoundaryColorizer>(BoundaryColorizer::Detection::NEIGBOUR_THRESHOLD, 40);
    case ColorizerId::UVW:
        return makeAuto<UvwColorizer>();
    case ColorizerId::PARTICLE_ID:
        return makeAuto<ParticleIdColorizer>(settings);
    case ColorizerId::COMPONENT_ID:
        return makeAuto<ComponentIdColorizer>(
            settings, Post::ComponentFlag::OVERLAP | Post::ComponentFlag::SORT_BY_MASS);
    case ColorizerId::BOUND_COMPONENT_ID:
        return makeAuto<ComponentIdColorizer>(
            settings, Post::ComponentFlag::ESCAPE_VELOCITY | Post::ComponentFlag::SORT_BY_MASS);
    case ColorizerId::AGGREGATE_ID:
        return makeAuto<AggregateIdColorizer>(settings);
    case ColorizerId::FLAG:
        return makeAuto<FlagColorizer>(settings);
    case ColorizerId::BEAUTY:
        return makeAuto<BeautyColorizer>();
    case ColorizerId::MARKER:
        return makeAuto<MarkerColorizer>(Rgba::gray(1.e6_f));
    default:
        QuantityId quantity = QuantityId(id);
        ASSERT(int(quantity) >= 0);

        Palette palette = getPalette(id);
        switch (getMetadata(quantity).expectedType) {
        case ValueEnum::INDEX:
            return makeAuto<TypedColorizer<Size>>(quantity, std::move(palette));
        case ValueEnum::SCALAR:
            return makeAuto<TypedColorizer<Float>>(quantity, std::move(palette));
        case ValueEnum::VECTOR:
            return makeAuto<TypedColorizer<Vector>>(quantity, std::move(palette));
        case ValueEnum::TRACELESS_TENSOR:
            return makeAuto<TypedColorizer<TracelessTensor>>(quantity, std::move(palette));
        case ValueEnum::SYMMETRIC_TENSOR:
            return makeAuto<TypedColorizer<SymmetricTensor>>(quantity, std::move(palette));
        default:
            NOT_IMPLEMENTED;
        }
    }
}

AutoPtr<IColorizer> Factory::getColorizer(const GuiSettings& settings, const ColorizerId id) {
    AutoPtr<IColorizer> colorizer = Sph::getColorizer(settings, id);
    Optional<Palette> palette = colorizer->getPalette();
    if (palette && Config::getPalette(colorizer->name(), palette.value())) {
        colorizer->setPalette(palette.value());
    }
    return colorizer;
}

class PaletteKey {
private:
    int key;

public:
    PaletteKey(const QuantityId id)
        : key(int(id)) {}

    PaletteKey(const ColorizerId id)
        : key(int(id)) {}

    bool operator<(const PaletteKey& other) const {
        return key < other.key;
    }
    bool operator==(const PaletteKey& other) const {
        return key == other.key;
    }
    bool operator!=(const PaletteKey& other) const {
        return key != other.key;
    }
};

struct PaletteDesc {
    Interval range;
    PaletteScale scale;
};

static FlatMap<PaletteKey, PaletteDesc> paletteDescs = {
    { QuantityId::DENSITY, { Interval(2650._f, 2750._f), PaletteScale::LINEAR } },
    { QuantityId::MASS, { Interval(0._f, 1.e10_f), PaletteScale::LINEAR } },
    { QuantityId::PRESSURE, { Interval(-1.e5_f, 1.e10_f), PaletteScale::HYBRID } },
    { QuantityId::ENERGY, { Interval(1._f, 1.e6_f), PaletteScale::LOGARITHMIC } },
    { QuantityId::DEVIATORIC_STRESS, { Interval(0._f, 1.e10_f), PaletteScale::LINEAR } },
    { QuantityId::DAMAGE, { Interval(0._f, 1._f), PaletteScale::LINEAR } },
    { QuantityId::VELOCITY_DIVERGENCE, { Interval(-0.1_f, 0.1_f), PaletteScale::LINEAR } },
    { QuantityId::VELOCITY_GRADIENT, { Interval(0._f, 1.e-3_f), PaletteScale::LINEAR } },
    { QuantityId::VELOCITY_ROTATION, { Interval(0._f, 4._f), PaletteScale::LINEAR } },
    { QuantityId::AV_STRESS, { Interval(0._f, 1.e8_f), PaletteScale::LINEAR } },
    { QuantityId::ANGULAR_FREQUENCY, { Interval(0._f, 1.e-3_f), PaletteScale::LINEAR } },
    { QuantityId::MOMENT_OF_INERTIA, { Interval(0._f, 1.e10_f), PaletteScale::LINEAR } },
    { QuantityId::STRAIN_RATE_CORRECTION_TENSOR, { Interval(0.5_f, 5._f), PaletteScale::LINEAR } },
    { QuantityId::EPS_MIN, { Interval(0._f, 1._f), PaletteScale::LINEAR } },
    { QuantityId::NEIGHBOUR_CNT, { Interval(50._f, 150._f), PaletteScale::LINEAR } },
    { ColorizerId::VELOCITY, { Interval(0.1_f, 100._f), PaletteScale::LOGARITHMIC } },
    { ColorizerId::ACCELERATION, { Interval(0.1_f, 100._f), PaletteScale::LOGARITHMIC } },
    { ColorizerId::MOVEMENT_DIRECTION, { Interval(0._f, 2._f * PI), PaletteScale::LINEAR } },
    { ColorizerId::RADIUS, { Interval(0._f, 1.e3_f), PaletteScale::LINEAR } },
    { ColorizerId::TOTAL_ENERGY, { Interval(1.e6_f, 1.e10_f), PaletteScale::LOGARITHMIC } },
    { ColorizerId::TEMPERATURE, { Interval(100._f, 1.e7_f), PaletteScale::LOGARITHMIC } },
    { ColorizerId::DENSITY_PERTURBATION, { Interval(-1.e-6_f, 1.e-6_f), PaletteScale::LINEAR } },
    { ColorizerId::DAMAGE_ACTIVATION, { Interval(2.e-4_f, 8.e-4_f), PaletteScale::LINEAR } },
    { ColorizerId::YIELD_REDUCTION, { Interval(0._f, 1._f), PaletteScale::LINEAR } },
    { ColorizerId::TOTAL_STRESS, { Interval(0._f, 1e6_f), PaletteScale::LINEAR } },
};

static Palette getDefaultPalette(const Interval range) {
    const float x0 = float(range.lower());
    const float dx = float(range.size());
    return Palette({ { x0, Rgba(0.f, 0.f, 0.6f) },
                       { x0 + 0.2f * dx, Rgba(0.1f, 0.1f, 0.1f) },
                       { x0 + 0.6f * dx, Rgba(0.9f, 0.9f, 0.9f) },
                       { x0 + 0.8f * dx, Rgba(1.f, 1.f, 0.f) },
                       { x0 + dx, Rgba(0.6f, 0.f, 0.f) } },
        PaletteScale::LINEAR);
}

Palette Factory::getPalette(const ColorizerId id) {
    const PaletteDesc desc = paletteDescs[id];
    const Interval range = desc.range;
    const PaletteScale scale = desc.scale;
    const float x0 = Float(range.lower());
    const float dx = Float(range.size());
    if (int(id) >= 0) {
        QuantityId quantity = QuantityId(id);
        switch (quantity) {
        case QuantityId::PRESSURE:
            ASSERT(x0 < -1.f);
            return Palette({ { x0, Rgba(0.3f, 0.3f, 0.8f) },
                               { -1.e4f, Rgba(0.f, 0.f, 0.2f) },
                               { 0.f, Rgba(0.2f, 0.2f, 0.2f) },
                               { 1.e4f, Rgba(0.8f, 0.8f, 0.8f) },
                               { 2.e4f, Rgba(1.f, 1.f, 0.2f) },
                               { x0 + dx, Rgba(0.5f, 0.f, 0.f) } },
                scale);
        case QuantityId::ENERGY:
            return Palette({ { x0, Rgba(0.1f, 0.1f, 0.1f) },
                               { x0 + 0.001f * dx, Rgba(0.1f, 0.1f, 1.f) },
                               { x0 + 0.01f * dx, Rgba(1.f, 0.f, 0.f) },
                               { x0 + 0.1f * dx, Rgba(1.0f, 0.6f, 0.4f) },
                               { x0 + dx, Rgba(1.f, 1.f, 0.f) } },
                scale);
        case QuantityId::DEVIATORIC_STRESS:
            return Palette({ { x0, Rgba(0.f, 0.f, 0.2f) },
                               { x0 + 0.1f * dx, Rgba(0.9f, 0.9f, 0.9f) },
                               { x0 + 0.25f * dx, Rgba(1.f, 1.f, 0.2f) },
                               { x0 + 0.5f * dx, Rgba(1.f, 0.5f, 0.f) },
                               { x0 + dx, Rgba(0.5f, 0.f, 0.f) } },
                scale);
        case QuantityId::DENSITY:
        case QuantityId::VELOCITY_LAPLACIAN:
        case QuantityId::FRICTION:
        case QuantityId::VELOCITY_GRADIENT_OF_DIVERGENCE:
            return Palette({ { x0, Rgba(0.4f, 0.f, 0.4f) },
                               { x0 + 0.3f * dx, Rgba(0.3_f, 0.3_f, 1._f) },
                               { x0 + 0.5f * dx, Rgba(0.9f, 0.9f, 0.9f) },
                               { x0 + 0.7f * dx, Rgba(1.f, 0.f, 0.f) },
                               { x0 + dx, Rgba(1.f, 1.f, 0.f) } },
                scale);
        case QuantityId::DAMAGE:
            return Palette({ { x0, Rgba(0.1f, 0.1f, 0.1f) }, { x0 + dx, Rgba(0.9f, 0.9f, 0.9f) } }, scale);
        case QuantityId::MASS:
            return Palette({ { x0, Rgba(0.1f, 0.1f, 0.1f) }, { x0 + dx, Rgba(0.9f, 0.9f, 0.9f) } }, scale);
        case QuantityId::VELOCITY_DIVERGENCE:
            ASSERT(x0 < 0._f);
            return Palette({ { x0, Rgba(0.3f, 0.3f, 0.8f) },
                               { 0.1f * x0, Rgba(0.f, 0.f, 0.2f) },
                               { 0.f, Rgba(0.2f, 0.2f, 0.2f) },
                               { 0.1f * (x0 + dx), Rgba(0.8f, 0.8f, 0.8f) },
                               { x0 + dx, Rgba(1.0f, 0.6f, 0.f) } },
                scale);
        case QuantityId::VELOCITY_GRADIENT:
            ASSERT(x0 == 0._f);
            return Palette({ { 0._f, Rgba(0.3f, 0.3f, 0.8f) },
                               { 0.01f * dx, Rgba(0.f, 0.f, 0.2f) },
                               { 0.05f * dx, Rgba(0.2f, 0.2f, 0.2f) },
                               { 0.2f * dx, Rgba(0.8f, 0.8f, 0.8f) },
                               { dx, Rgba(1.0f, 0.6f, 0.f) } },
                scale);
        case QuantityId::ANGULAR_FREQUENCY:
            ASSERT(x0 == 0._f);
            return Palette({ { 0._f, Rgba(0.3f, 0.3f, 0.8f) },
                               { 0.25f * dx, Rgba(0.f, 0.f, 0.2f) },
                               { 0.5f * dx, Rgba(0.2f, 0.2f, 0.2f) },
                               { 0.75f * dx, Rgba(0.8f, 0.8f, 0.8f) },
                               { dx, Rgba(1.0f, 0.6f, 0.f) } },
                scale);
        case QuantityId::STRAIN_RATE_CORRECTION_TENSOR:
            ASSERT(x0 > 0._f);
            return Palette({ { x0, Rgba(0.f, 0.f, 0.5f) },
                               { x0 + 0.01f, Rgba(0.1f, 0.1f, 0.1f) },
                               { x0 + 0.6f * dx, Rgba(0.9f, 0.9f, 0.9f) },
                               { x0 + dx, Rgba(0.6f, 0.0f, 0.0f) } },
                scale);
        case QuantityId::AV_BALSARA:
            return Palette({ { x0, Rgba(0.1f, 0.1f, 0.1f) }, { x0 + dx, Rgba(0.9f, 0.9f, 0.9f) } }, scale);
        case QuantityId::EPS_MIN:
            return Palette({ { x0, Rgba(0.1f, 0.1f, 1.f) },
                               { x0 + 0.5f * dx, Rgba(0.7f, 0.7f, 0.7f) },
                               { x0 + dx, Rgba(1.f, 0.1f, 0.1f) } },
                scale);
        case QuantityId::MOMENT_OF_INERTIA:
            return Palette({ { x0, Rgba(0.1f, 0.1f, 1.f) },
                               { x0 + 0.5f * dx, Rgba(0.7f, 0.7f, 0.7f) },
                               { x0 + dx, Rgba(1.f, 0.1f, 0.1f) } },
                scale);
        default:
            return getDefaultPalette(Interval(x0, x0 + dx));
        }
    } else {
        switch (id) {
        case ColorizerId::VELOCITY:
            return Palette({ { x0, Rgba(0.5f, 0.5f, 0.5f) },
                               { x0 + 0.001f * dx, Rgba(0.0f, 0.0f, 0.2f) },
                               { x0 + 0.01f * dx, Rgba(0.0f, 0.0f, 1.0f) },
                               { x0 + 0.1f * dx, Rgba(1.0f, 0.0f, 0.2f) },
                               { x0 + dx, Rgba(1.0f, 1.0f, 0.2f) } },
                scale);
        case ColorizerId::ACCELERATION:
            return Palette({ { x0, Rgba(0.5f, 0.5f, 0.5f) },
                               { x0 + 0.001f * dx, Rgba(0.0f, 0.0f, 0.2f) },
                               { x0 + 0.01f * dx, Rgba(0.0f, 0.0f, 1.0f) },
                               { x0 + 0.1f * dx, Rgba(1.0f, 0.0f, 0.2f) },
                               { x0 + dx, Rgba(1.0f, 1.0f, 0.2f) } },
                scale);
        case ColorizerId::MOVEMENT_DIRECTION:
            ASSERT(range == Interval(0.f, 2.f * PI)); // in radians
            return Palette({ { 0.f, Rgba(0.1f, 0.1f, 1.f) },
                               { PI / 3.f, Rgba(1.f, 0.1f, 1.f) },
                               { 2.f * PI / 3.f, Rgba(1.f, 0.1f, 0.1f) },
                               { 3.f * PI / 3.f, Rgba(1.f, 1.f, 0.1f) },
                               { 4.f * PI / 3.f, Rgba(0.1f, 1.f, 0.1f) },
                               { 5.f * PI / 3.f, Rgba(0.1f, 1.f, 1.f) },
                               { 2.f * PI, Rgba(0.1f, 0.1f, 1.f) } },
                scale);
        case ColorizerId::DENSITY_PERTURBATION:
            return Palette({ { x0, Rgba(0.1f, 0.1f, 1.f) },
                               { x0 + 0.5f * dx, Rgba(0.7f, 0.7f, 0.7f) },
                               { x0 + dx, Rgba(1.f, 0.1f, 0.1f) } },
                scale);
        case ColorizerId::TOTAL_ENERGY:
            return Palette({ { x0, Rgba(0.f, 0.f, 0.6f) },
                               { x0 + 0.01f * dx, Rgba(0.1f, 0.1f, 0.1f) },
                               { x0 + 0.05f * dx, Rgba(0.9f, 0.9f, 0.9f) },
                               { x0 + 0.2f * dx, Rgba(1.f, 1.f, 0.f) },
                               { x0 + dx, Rgba(0.6f, 0.f, 0.f) } },
                scale);
        case ColorizerId::TEMPERATURE:
            return Palette({ { x0, Rgba(0.1f, 0.1f, 0.1f) },
                               { x0 + 0.001f * dx, Rgba(0.1f, 0.1f, 1.f) },
                               { x0 + 0.01f * dx, Rgba(1.f, 0.f, 0.f) },
                               { x0 + 0.1f * dx, Rgba(1.0f, 0.6f, 0.4f) },
                               { x0 + dx, Rgba(1.f, 1.f, 0.f) } },
                scale);
        case ColorizerId::YIELD_REDUCTION:
            return Palette({ { 0._f, Rgba(0.1f, 0.1f, 0.1f) }, { 1._f, Rgba(0.9f, 0.9f, 0.9f) } }, scale);
        case ColorizerId::DAMAGE_ACTIVATION:
            return Palette({ { x0, Rgba(0.1f, 0.1f, 1.f) },
                               { x0 + 0.5f * dx, Rgba(0.7f, 0.7f, 0.7f) },
                               { x0 + dx, Rgba(1.f, 0.1f, 0.1f) } },
                scale);
        case ColorizerId::RADIUS:
            return Palette({ { x0, Rgba(0.1f, 0.1f, 1.f) },
                               { x0 + 0.5f * dx, Rgba(0.7f, 0.7f, 0.7f) },
                               { x0 + dx, Rgba(1.f, 0.1f, 0.1f) } },
                scale);
        default:
            return getDefaultPalette(Interval(x0, x0 + dx));
        }
    }
}

NAMESPACE_SPH_END
