#include "run/VirtualSettings.h"

NAMESPACE_SPH_BEGIN

EntryControl& EntryControl::setTooltip(const String& newTooltip) {
    tooltip = newTooltip;
    return *this;
}

EntryControl& EntryControl::setUnits(const Float newMult) {
    mult = newMult;
    return *this;
}

EntryControl& EntryControl::setEnabler(const Enabler& newEnabler) {
    enabler = newEnabler;
    return *this;
}

EntryControl& EntryControl::setFallback(const Fallback& newFallback) {
    fallback = newFallback;
    return *this;
}

EntryControl& EntryControl::addAccessor(const SharedToken& owner, const Accessor& accessor) {
    accessors.insert(owner, accessor);
    return *this;
}

EntryControl& EntryControl::setValidator(const Validator& newValidator) {
    validator = newValidator;
    return *this;
}

EntryControl& EntryControl::setSideEffect() {
    sideEffect = true;
    return *this;
}

EntryControl& EntryControl::setPathType(const PathType& newType) {
    pathType = newType;
    return *this;
}

EntryControl& EntryControl::setFileFormats(Array<FileFormat>&& formats) {
    fileFormats = std::move(formats);
    return *this;
}

Optional<IVirtualEntry::PathType> EntryControl::getPathType() const {
    return pathType;
}

Array<IVirtualEntry::FileFormat> EntryControl::getFileFormats() const {
    return fileFormats.clone();
}

bool EntryControl::isValid(const Value& value) const {
    return !validator || validator(value);
}

bool EntryControl::enabled() const {
    return enabler ? enabler() : true;
}

String EntryControl::getTooltip() const {
    return tooltip;
}

bool EntryControl::hasSideEffect() const {
    return sideEffect;
}

bool EntryControl::set(const Value& value) {
    if (!this->isValid(value)) {
        return false;
    }
    this->setImpl(value);
    accessors(value);
    return true;
}

bool EntryControl::setFromFallback() {
    fallback.callIfNotNull();
    return true;
}

void VirtualSettings::set(const String& key, const IVirtualEntry::Value& value) {
    for (auto& category : categories) {
        auto entry = category.value().entries.tryGet(key);
        if (entry) {
            entry.value()->set(value);
            return;
        }
    }
    throw InvalidSetup("Key '" + key + "' not found");
}

IVirtualEntry::Value VirtualSettings::get(const String& key) const {
    for (auto& category : categories) {
        auto entry = category.value().entries.tryGet(key);
        if (entry) {
            return entry.value()->get();
        }
    }
    throw InvalidSetup("Key '" + key + "' not found");
}

VirtualSettings::Category& VirtualSettings::addCategory(const String& name) {
    return categories.insert(name, Category{});
}

void VirtualSettings::enumerate(const IEntryProc& proc) {
    for (auto& category : categories) {
        proc.onCategory(category.key());
        for (auto& entry : category.value().entries) {
            proc.onEntry(entry.key(), *entry.value());
        }
    }
}

static Array<IVirtualEntry::FileFormat> getFormats(const IoCapability capability) {
    Array<IVirtualEntry::FileFormat> formats;
    for (IoEnum id : EnumMap::getAll<IoEnum>()) {
        if (getIoCapabilities(id).has(capability)) {
            IVirtualEntry::FileFormat format;
            format.description = getIoDescription(id);
            format.extension = getIoExtension(id).value();
            formats.push(format);
        }
    }
    return formats;
}

Array<IVirtualEntry::FileFormat> getInputFormats() {
    return getFormats(IoCapability::INPUT);
}

Array<IVirtualEntry::FileFormat> getOutputFormats() {
    return getFormats(IoCapability::OUTPUT);
}

NAMESPACE_SPH_END
