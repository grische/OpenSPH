#include "sph/boundary/Boundary.h"
#include "objects/finders/PeriodicFinder.h"
#include "objects/geometry/Domain.h"
#include "objects/utility/Iterator.h"
#include "quantities/Attractor.h"
#include "quantities/IMaterial.h"
#include "quantities/Iterate.h"
#include "sph/initial/Distribution.h"
#include "sph/kernel/Kernel.h"
#include "system/Factory.h"
#include "thread/Scheduler.h"
#include "timestepping/ISolver.h"

NAMESPACE_SPH_BEGIN

//-----------------------------------------------------------------------------------------------------------
// GhostParticles implementation
//-----------------------------------------------------------------------------------------------------------

void GhostParticlesData::remove(ArrayView<const Size> idxs) {
    for (Size i = 0; i < ghosts.size();) {
        const Size pi = ghosts[i].index;
        if (std::binary_search(idxs.begin(), idxs.end(), pi)) {
            std::swap(ghosts[i], ghosts.back());
            ghosts.pop();
        } else {
            ++i;
        }
    }
}

GhostParticles::GhostParticles(SharedPtr<IDomain> domain, const Float searchRadius, const Float minimalDist)
    : domain(std::move(domain)) {
    SPH_ASSERT(this->domain);
    params.searchRadius = searchRadius;
    params.minimalDist = minimalDist;
}

GhostParticles::GhostParticles(SharedPtr<IDomain> domain, const RunSettings& settings)
    : domain(std::move(domain)) {
    SPH_ASSERT(this->domain);
    params.searchRadius = Factory::getKernel<3>(settings).radius();
    params.minimalDist = settings.get<Float>(RunSettingsId::DOMAIN_GHOST_MIN_DIST);
}

void GhostParticles::initialize(Storage& storage) {
    // note that there is no need to also add particles to dependent storages as we remove ghosts in
    // finalization; however, it might cause problems when there is another BC or equation that DOES need to
    // propagate to dependents. So far no such thing is used, but it would have to be done carefully in case
    // multiple objects add or remove particles from the storage.

    storage.setUserData(nullptr); // clear previous data

    SPH_ASSERT(ghosts.empty() && ghostIdxs.empty());

    // project particles outside of the domain on the boundary
    Array<Vector>& r = storage.getValue<Vector>(QuantityId::POSITION);
    domain->project(r);

    // find particles close to boundary and create necessary ghosts
    domain->addGhosts(r, ghosts, params.searchRadius, params.minimalDist);

    // extract the indices for duplication
    Array<Size> idxs;
    for (Ghost& g : ghosts) {
        idxs.push(g.index);
    }
    // duplicate all particles that create ghosts to make sure we have correct materials in the storage
    ghostIdxs = storage.duplicate(idxs);

    // set correct positions of ghosts
    SPH_ASSERT(ghostIdxs.size() == ghosts.size());
    for (Size i = 0; i < ghosts.size(); ++i) {
        const Size ghostIdx = ghostIdxs[i];
        r[ghostIdx] = ghosts[i].position;
    }

    // reflect velocities (only if we have velocities, GP are also used in Diehl's distribution, for example)
    if (storage.has<Vector>(QuantityId::POSITION, OrderEnum::SECOND)) {
        Array<Vector>& v = storage.getDt<Vector>(QuantityId::POSITION);
        for (Size i = 0; i < ghosts.size(); ++i) {
            const Size ghostIdx = ghostIdxs[i];
            // offset between particle and its ghost
            const Vector deltaR = r[ghosts[i].index] - ghosts[i].position;
            SPH_ASSERT(getLength(deltaR) > 0._f);
            const Vector normal = getNormalized(deltaR);
            const Float perp = dot(normal, v[ghosts[i].index]);
            // mirror vector by copying parallel component and inverting perpendicular component
            const Vector v0 = v[ghostIdx] - 2._f * normal * perp;
            if (ghostVelocity) {
                v[ghostIdx] = ghostVelocity(ghosts[i].position).valueOr(v0);
            } else {
                v[ghostIdx] = v0;
            }
            SPH_ASSERT(getLength(v[ghostIdx]) < 1.e50_f);
        }
    }

    // set flag to some special value to separate the bodies
    if (storage.has(QuantityId::FLAG)) {
        ArrayView<Size> flag = storage.getValue<Size>(QuantityId::FLAG);
        for (Size i = 0; i < ghosts.size(); ++i) {
            const Size ghostIdx = ghostIdxs[i];
            flag[ghostIdx] = FLAG;
        }
    }

    SPH_ASSERT(storage.isValid());

    particleCnt = storage.getParticleCnt();
}

void GhostParticles::setVelocityOverride(Function<Optional<Vector>(const Vector& r)> newGhostVelocity) {
    ghostVelocity = newGhostVelocity;
}

void GhostParticles::finalize(Storage& storage) {
    SPH_ASSERT(storage.getParticleCnt() == particleCnt,
        "Solver changed the number of particles. This is currently not consistent with the implementation of "
        "GhostParticles");

    // remove ghosts by indices
    storage.remove(ghostIdxs);
    ghostIdxs.clear();

    storage.setUserData(makeShared<GhostParticlesData>(std::move(ghosts)));
}

//-----------------------------------------------------------------------------------------------------------
// FixedParticles implementation
//-----------------------------------------------------------------------------------------------------------

FixedParticles::FixedParticles(const RunSettings& settings, Params&& params)
    : fixedParticles(std::move(params.material)) {
    SPH_ASSERT(isReal(params.thickness));
    Box box = params.domain->getBoundingBox();
    box.extend(box.lower() - Vector(params.thickness));
    box.extend(box.upper() + Vector(params.thickness));

    // We need to fill a layer close to the boundary with dummy particles. IDomain interface does not provide
    // a way to construct enlarged domain, so we need a more general approach - construct a block domain using
    // the bounding box, fill it with particles, and then remove all particles that are inside the original
    // domain or too far from the boundary. This may be inefficient for some obscure domain, but otherwise it
    // works fine.
    BlockDomain boundingDomain(box.center(), box.size());
    /// \todo generalize, we assume that kernel radius = 2 and don't take eta into account
    const Size n = Size(box.volume() / pow<3>(0.5_f * params.thickness));
    Array<Vector> dummies = params.distribution->generate(SEQUENTIAL, n, boundingDomain);
    // remove all particles inside the actual domain or too far away
    /*Array<Float> distances;
    params.domain->getDistanceToBoundary(dummies, distances);*/
    for (Size i = 0; i < dummies.size();) {
        if (params.domain->contains(dummies[i])) {
            // if (distances[i] >= 0._f || distances[i] < -params.thickness) {
            dummies.remove(i);
        } else {
            ++i;
        }
    }
    // although we never use the derivatives, the second order is needed to properly merge the storages
    fixedParticles.insert<Vector>(QuantityId::POSITION, OrderEnum::SECOND, std::move(dummies));

    // create all quantities
    MaterialView mat = fixedParticles.getMaterial(0);
    const Float rho0 = mat->getParam<Float>(BodySettingsId::DENSITY);
    const Float m0 = rho0 * box.volume() / fixedParticles.getParticleCnt();
    fixedParticles.insert<Float>(QuantityId::MASS, OrderEnum::ZERO, m0);
    // use negative flag to separate the dummy particles from the real ones
    fixedParticles.insert<Size>(QuantityId::FLAG, OrderEnum::ZERO, Size(-1));

    AutoPtr<ISolver> solver = Factory::getSolver(SEQUENTIAL, settings);
    solver->create(fixedParticles, mat);
    MaterialInitialContext context(settings);
    mat->create(fixedParticles, context);
}

void FixedParticles::initialize(Storage& storage) {
    // add all fixed particles into the storage
    storage.merge(fixedParticles.clone(VisitorEnum::ALL_BUFFERS));
    SPH_ASSERT(storage.isValid());
    SPH_ASSERT(storage.getValue<TracelessTensor>(QuantityId::DEVIATORIC_STRESS).size() ==
               storage.getValue<Vector>(QuantityId::POSITION).size());
}

void FixedParticles::finalize(Storage& storage) {
    // remove all fixed particles (particles with flag -1) from the storage
    ArrayView<const Size> flag = storage.getValue<Size>(QuantityId::FLAG);
    Array<Size> toRemove;
    for (Size i = 0; i < flag.size(); ++i) {
        if (flag[i] == Size(-1)) {
            toRemove.push(i);
        }
    }

    storage.remove(toRemove, Storage::IndicesFlag::INDICES_SORTED);
    SPH_ASSERT(storage.isValid());
}

//-----------------------------------------------------------------------------------------------------------
// FrozenParticles implementation
//-----------------------------------------------------------------------------------------------------------

FrozenParticles::FrozenParticles() = default;

FrozenParticles::~FrozenParticles() = default;

FrozenParticles::FrozenParticles(SharedPtr<IDomain> domain, const Float radius)
    : domain(std::move(domain))
    , radius(radius) {}

/// Adds body ID particles of which shall be frozen by boundary conditions.
void FrozenParticles::freeze(const Size flag) {
    frozen.insert(flag);
}

/// Remove a body from the list of frozen bodies. If the body is not on the list, nothing happens.
void FrozenParticles::thaw(const Size flag) {
    frozen.erase(flag);
}

void FrozenParticles::finalize(Storage& storage) {
    ArrayView<Vector> r, v, dv;
    tie(r, v, dv) = storage.getAll<Vector>(QuantityId::POSITION);

    idxs.clear();
    if (domain) {
        // project particles outside of the domain onto the boundary
        domain->getSubset(r, idxs, SubsetType::OUTSIDE);
        domain->project(r, idxs.view());

        // freeze particles close to the boundary
        domain->getDistanceToBoundary(r, distances);
        for (Size i = 0; i < r.size(); ++i) {
            SPH_ASSERT(distances[i] >= -EPS);
            if (distances[i] < radius * r[i][H]) {
                idxs.push(i);
            }
        }
    }

    if (!frozen.empty()) {
        // find all frozen particles by their body flag
        ArrayView<Size> flags = storage.getValue<Size>(QuantityId::FLAG);
        for (Size i = 0; i < r.size(); ++i) {
            if (frozen.find(flags[i]) != frozen.end()) {
                idxs.push(i); // this might add particles already frozen by boundary, but it doesn't matter
            }
        }
    }

    // set all highest derivatives of flagged particles to zero
    iterate<VisitorEnum::HIGHEST_DERIVATIVES>(storage, [this](QuantityId, auto& d2f) {
        using T = typename std::decay_t<decltype(d2f)>::Type;
        for (Size i : idxs) {
            d2f[i] = T(0._f);
        }
    });
}

//-----------------------------------------------------------------------------------------------------------
// WindTunnel implementation
//-----------------------------------------------------------------------------------------------------------

WindTunnel::WindTunnel(SharedPtr<IDomain> domain, const Float radius)
    : FrozenParticles(std::move(domain), radius) {}

void WindTunnel::finalize(Storage& storage) {
    // clear derivatives of particles close to boundary
    FrozenParticles::finalize(storage);

    // remove particles outside of the domain
    Array<Size> toRemove;
    Array<Vector>& r = storage.getValue<Vector>(QuantityId::POSITION);
    for (Size i = 0; i < r.size(); ++i) {
        if (!this->domain->contains(r[i])) {
            toRemove.push(i);
        }
    }
    iterate<VisitorEnum::ALL_BUFFERS>(storage, [&toRemove](auto& v) { v.remove(toRemove); });

    // find z positions of upper two layers of particles
    /* Float z1 = -INFTY, z2 = -INFTY;
     for (Size i = 0; i < r.size(); ++i) {
         if (r[i][Z] > z1) {
             z2 = z1;
             z1 = r[i][Z];
         }
     }
     SPH_ASSERT(z2 > -INFTY && z1 > z2 + 0.1_f * r[0][H]);
     const Float dz = z1 - z2;
     if (z1 + 2._f * dz < this->domain->getBoundingBox().upper()[Z]) {
         // find indices of upper two layers
         Array<Size> idxs;
         const Size size = r.size();
         for (Size i = 0; i < size; ++i) {
             if (r[i][Z] >= z2) {
                 idxs.push(i);
             }
         }
         SPH_ASSERT(!idxs.empty());
         // copy all quantities
         iterate<VisitorEnum::ALL_BUFFERS>(storage, [&idxs](auto& v) {
             for (Size i : idxs) {
                 auto cloned = v[i];
                 v.push(cloned);
             }
         });
         // move created particles by 2dz
         const Vector offset(0._f, 0._f, 2._f * dz);
         for (Size i = size; i < r.size(); ++i) {
             r[i] += offset;
         }
     }*/
    SPH_ASSERT(storage.isValid());
}

//-----------------------------------------------------------------------------------------------------------
// PeriodicBoundary implementation
//-----------------------------------------------------------------------------------------------------------

PeriodicBoundary::PeriodicBoundary(const Box& domain)
    : domain(domain) {}

static const StaticArray<Vector, 3> UNIT = {
    Vector(1._f, 0._f, 0._f),
    Vector(0._f, 1._f, 0._f),
    Vector(0._f, 0._f, 1._f),
};

void PeriodicBoundary::initialize(Storage& storage) {
    storage.setUserData(nullptr); // clear previous data
    SPH_ASSERT(ghostIdxs.empty() && ghosts.empty());

    Array<Size> duplIdxs;
    Array<Vector>& r = storage.getValue<Vector>(QuantityId::POSITION);
    const Float radius = 2._f;

    for (Size i = 0; i < r.size(); ++i) {
        // move particles outside of the domain to the other side
        const Vector lowerFlags(int(r[i][X] < domain.lower()[X]),
            int(r[i][Y] < domain.lower()[Y]),
            int(r[i][Z] < domain.lower()[Z]));
        const Vector upperFlags(int(r[i][X] > domain.upper()[X]),
            int(r[i][Y] > domain.upper()[Y]),
            int(r[i][Z] > domain.upper()[Z]));

        r[i] = setH(r[i] + domain.size() * (lowerFlags - upperFlags), r[i][H]);

        // for particles close to the boundary, add ghosts
        for (Size j = 0; j < 3; ++j) {
            if (r[i][j] < domain.lower()[j] + radius * r[i][H]) {
                duplIdxs.push(i);
                ghosts.push(Ghost{ r[i] + domain.size()[j] * UNIT[j], i });
            }
            if (r[i][j] > domain.upper()[j] - radius * r[i][H]) {
                duplIdxs.push(i);
                ghosts.push(Ghost{ r[i] - domain.size()[j] * UNIT[j], i });
            }
        }
    }

    // handle attractors
    for (Attractor& a : storage.getAttractors()) {
        Vector& p = a.position;
        const Vector lowerFlags(
            int(p[X] < domain.lower()[X]), int(p[Y] < domain.lower()[Y]), int(p[Z] < domain.lower()[Z]));
        const Vector upperFlags(
            int(p[X] > domain.upper()[X]), int(p[Y] > domain.upper()[Y]), int(p[Z] > domain.upper()[Z]));

        p += domain.size() * (lowerFlags - upperFlags);
    }

    ghostIdxs = storage.duplicate(duplIdxs);
    SPH_ASSERT(ghostIdxs.size() == duplIdxs.size());

    for (Size i = 0; i < ghostIdxs.size(); ++i) {
        r[ghostIdxs[i]] = ghosts[i].position;
        r[ghostIdxs[i]][H] = r[ghosts[i].index][H];
    }

    // set flag to some special value to separate the bodies
    if (storage.has(QuantityId::FLAG)) {
        ArrayView<Size> flag = storage.getValue<Size>(QuantityId::FLAG);
        for (Size i = 0; i < ghosts.size(); ++i) {
            flag[ghostIdxs[i]] = Size(-1);
        }
    }
}

void PeriodicBoundary::finalize(Storage& storage) {
    storage.remove(ghostIdxs);
    ghostIdxs.clear();

    storage.setUserData(makeShared<GhostParticlesData>(std::move(ghosts)));
}

//-----------------------------------------------------------------------------------------------------------
// SymmetricBoundary implementation
//-----------------------------------------------------------------------------------------------------------

void SymmetricBoundary::initialize(Storage& storage) {
    storage.setUserData(nullptr); // clear previous data
    SPH_ASSERT(ghostIdxs.empty() && ghosts.empty());

    Array<Size> duplIdxs;
    Array<Vector>& r = storage.getValue<Vector>(QuantityId::POSITION);
    const Float radius = 2._f;

    for (Size i = 0; i < r.size(); ++i) {
        if (r[i][Z] < 0._f) {
            r[i][Z] = 0.1_f * r[i][H];
        }

        // for particles close to the boundary, add ghosts
        for (Size j = 0; j < 3; ++j) {
            if (r[i][Z] < radius * r[i][H]) {
                duplIdxs.push(i);
                ghosts.push(Ghost{ r[i] - Vector(0, 0, 2._f * r[i][Z]), i });
            }
        }
    }

    ghostIdxs = storage.duplicate(duplIdxs);
    SPH_ASSERT(ghostIdxs.size() == duplIdxs.size());

    for (Size i = 0; i < ghostIdxs.size(); ++i) {
        r[ghostIdxs[i]] = ghosts[i].position;
    }
}

void SymmetricBoundary::finalize(Storage& storage) {
    storage.remove(ghostIdxs);
    ghostIdxs.clear();

    storage.setUserData(makeShared<GhostParticlesData>(std::move(ghosts)));
}

//-----------------------------------------------------------------------------------------------------------
// KillEscapersBoundary implementation
//-----------------------------------------------------------------------------------------------------------

KillEscapersBoundary::KillEscapersBoundary(SharedPtr<IDomain> domain)
    : domain(domain) {}

void KillEscapersBoundary::initialize(Storage& storage) {
    ArrayView<const Vector> r = storage.getValue<Vector>(QuantityId::POSITION);
    Array<Size> toRemove;
    for (Size i = 0; i < r.size(); ++i) {
        if (!domain->contains(r[i])) {
            toRemove.push(i);
        }
    }
    storage.remove(toRemove, Storage::IndicesFlag::INDICES_SORTED | Storage::IndicesFlag::PROPAGATE);
}

void KillEscapersBoundary::finalize(Storage& UNUSED(storage)) {}

//-----------------------------------------------------------------------------------------------------------
// Projection1D implementation
//----------------------------------------------------------------------------------------------------------

Projection1D::Projection1D(const Interval& domain)
    : domain(domain) {}

void Projection1D::finalize(Storage& storage) {
    ArrayView<Vector> dv;
    tie(r, v, dv) = storage.getAll<Vector>(QuantityId::POSITION);
    for (Size i = 0; i < r.size(); ++i) {
        // throw away y and z, keep h
        r[i] = Vector(domain.clamp(r[i][0]), 0._f, 0._f, r[i][H]);
        v[i] = Vector(v[i][0], 0._f, 0._f);
    }
    // To get fixed boundary conditions at ends, we need to null all derivatives of first few and last few
    // particles. Number of particles depends on smoothing length.
    iterate<VisitorEnum::FIRST_ORDER>(storage, [](const QuantityId UNUSED(id), auto&& UNUSED(v), auto&& dv) {
        using Type = typename std::decay_t<decltype(dv)>::Type;
        const Size s = dv.size();
        for (Size i : { 0u, 1u, 2u, 3u, 4u, s - 4, s - 3, s - 2, s - 1 }) {
            dv[i] = Type(0._f);
        }
    });
    iterate<VisitorEnum::SECOND_ORDER>(
        storage, [](const QuantityId UNUSED(id), auto&& UNUSED(v), auto&& dv, auto&& d2v) {
            using Type = typename std::decay_t<decltype(dv)>::Type;
            const Size s = dv.size();
            for (Size i : { 0u, 1u, 2u, 3u, 4u, s - 4, s - 3, s - 2, s - 1 }) {
                dv[i] = Type(0._f);
                d2v[i] = Type(0._f);
            }
        });
}

NAMESPACE_SPH_END
