#include "quantities/Particle.h"
#include "quantities/Quantity.h"
#include "quantities/Storage.h"

NAMESPACE_SPH_BEGIN

struct ParticleVisitor {
    FlatMap<QuantityId, Particle::InternalQuantityData>& data;

    template <typename TValue>
    void visit(const QuantityId id, const Quantity& q, const Size idx) {
        const auto& values = q.getAll<TValue>();
        switch (q.getOrderEnum()) {
        case OrderEnum::SECOND:
            data[id].d2t = values[2][idx];
            SPH_FALLTHROUGH
        case OrderEnum::FIRST:
            data[id].dt = values[1][idx];
            SPH_FALLTHROUGH
        case OrderEnum::ZERO:
            data[id].value = values[0][idx];
            break;
        default:
            NOT_IMPLEMENTED;
        }
    }
};


Particle::Particle(const Storage& storage, const Size idx)
    : idx(idx) {
    for (ConstStorageElement i : storage.getQuantities()) {
        quantities.insert(i.id, InternalQuantityData{});

        ParticleVisitor visitor{ quantities };
        dispatch(i.quantity.getValueEnum(), visitor, i.id, i.quantity, idx);
    }
}

Particle::Particle(const QuantityId id, const Dynamic& value, const Size idx)
    : idx(idx) {
    quantities.insert(id, InternalQuantityData{});
    quantities[id].value = value;
}

Particle::Particle(const Particle& other) {
    *this = other;
}

Particle::Particle(Particle&& other) {
    *this = std::move(other);
}

Particle& Particle::operator=(const Particle& other) {
    quantities = other.quantities.clone();
    material = other.material.clone();
    idx = other.idx;
    return *this;
}

Particle& Particle::operator=(Particle&& other) {
    quantities = std::move(other.quantities);
    material = std::move(other.material);
    idx = other.idx;
    return *this;
}

Particle& Particle::addValue(const QuantityId id, const Dynamic& value) {
    if (!quantities.contains(id)) {
        quantities.insert(id, InternalQuantityData{});
    }
    quantities[id].value = value;
    return *this;
}

Particle& Particle::addDt(const QuantityId id, const Dynamic& value) {
    if (!quantities.contains(id)) {
        quantities.insert(id, InternalQuantityData{});
    }
    quantities[id].dt = value;
    return *this;
}

Particle& Particle::addD2t(const QuantityId id, const Dynamic& value) {
    if (!quantities.contains(id)) {
        quantities.insert(id, InternalQuantityData{});
    }
    quantities[id].d2t = value;
    return *this;
}

Particle& Particle::addParameter(const BodySettingsId id, const Dynamic& value) {
    if (!material.contains(id)) {
        material.insert(id, NOTHING);
    }
    material[id] = value;
    return *this;
}


Dynamic Particle::getValue(const QuantityId id) const {
    Optional<const InternalQuantityData&> quantity = quantities.tryGet(id);
    SPH_ASSERT(quantity);
    return quantity->value;
}

Dynamic Particle::getDt(const QuantityId id) const {
    Optional<const InternalQuantityData&> quantity = quantities.tryGet(id);
    SPH_ASSERT(quantity);
    return quantity->dt;
}

Dynamic Particle::getD2t(const QuantityId id) const {
    Optional<const InternalQuantityData&> quantity = quantities.tryGet(id);
    SPH_ASSERT(quantity);
    return quantity->d2t;
}

Dynamic Particle::getParameter(const BodySettingsId id) const {
    Optional<const Dynamic&> value = material.tryGet(id);
    SPH_ASSERT(value);
    return value.value();
}


Particle::QuantityIterator::QuantityIterator(const ActIterator iterator, Badge<Particle>)
    : iter(iterator) {}

Particle::QuantityIterator& Particle::QuantityIterator::operator++() {
    ++iter;
    return *this;
}

Particle::QuantityData Particle::QuantityIterator::operator*() const {
    const InternalQuantityData& internal = iter->value();
    DynamicId type;
    if (internal.value) {
        type = internal.value.getType();
        SPH_ASSERT(internal.dt.empty() || internal.dt.getType() == type);
        SPH_ASSERT(internal.d2t.empty() || internal.d2t.getType() == type);
    } else if (internal.dt) {
        type = internal.dt.getType();
        SPH_ASSERT(internal.d2t.empty() || internal.d2t.getType() == type);
    } else {
        SPH_ASSERT(internal.d2t);
        type = internal.d2t.getType();
    }
    return { iter->key(), type, internal.value, internal.dt, internal.d2t };
}

bool Particle::QuantityIterator::operator!=(const QuantityIterator& other) const {
    return iter != other.iter;
}

Particle::QuantitySequence::QuantitySequence(const Particle& particle)
    : first(particle.quantities.begin(), {})
    , last(particle.quantities.end(), {}) {}

Particle::QuantityIterator Particle::QuantitySequence::begin() const {
    return first;
}

Particle::QuantityIterator Particle::QuantitySequence::end() const {
    return last;
}

Particle::QuantitySequence Particle::getQuantities() const {
    return QuantitySequence(*this);
}


Particle::ParamIterator::ParamIterator(const ActIterator iterator, Badge<Particle>)
    : iter(iterator) {}

Particle::ParamIterator& Particle::ParamIterator::operator++() {
    ++iter;
    return *this;
}

Particle::ParamData Particle::ParamIterator::operator*() const {
    return { iter->key(), iter->value() };
}

bool Particle::ParamIterator::operator!=(const ParamIterator& other) const {
    return iter != other.iter;
}


Particle::ParamSequence::ParamSequence(const Particle& particle)
    : first(particle.material.begin(), {})
    , last(particle.material.end(), {}) {}

Particle::ParamIterator Particle::ParamSequence::begin() const {
    return first;
}

Particle::ParamIterator Particle::ParamSequence::end() const {
    return last;
}

Particle::ParamSequence Particle::getParameters() const {
    return ParamSequence(*this);
}

NAMESPACE_SPH_END
