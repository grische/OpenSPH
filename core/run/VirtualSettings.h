#pragma once

/// \file VirtualSettings.h
/// \brief Object providing connection between component parameters and values exposed to the user
/// \author Pavel Sevecek (sevecek at sirrah.troja.mff.cuni.cz)
/// \date 2016-2021

#include "io/Path.h"
#include "objects/containers/CallbackSet.h"
#include "objects/containers/UnorderedMap.h"
#include "system/Settings.h"

NAMESPACE_SPH_BEGIN

/// \brief Provides an interface for implementing new types of entries.
///
/// Class \ref IVirtualEntry can store basic types, such as bool, int, String, etc. When necessary to
/// store more complex data, the user can implement \ref IExtraEntry interface and store the data using \ref
/// ExtraEntry wrapper.
class IExtraEntry : public Polymorphic {
public:
    virtual String toString() const = 0;

    virtual void fromString(const String& s) = 0;

    /// \todo use ClonePtr instead
    virtual AutoPtr<IExtraEntry> clone() const = 0;
};

/// \brief Copyable wrapper of a \ref IExtraEntry.
class ExtraEntry {
public:
    AutoPtr<IExtraEntry> entry;

public:
    ExtraEntry() = default;

    explicit ExtraEntry(AutoPtr<IExtraEntry>&& entry)
        : entry(std::move(entry)) {}

    ExtraEntry(const ExtraEntry& other)
        : entry(other.entry->clone()) {}

    ExtraEntry& operator=(const ExtraEntry& other) {
        entry = other.entry->clone();
        return *this;
    }

    String toString() const {
        return entry->toString();
    }

    void fromString(const String& s) const {
        entry->fromString(s);
    }

    RawPtr<IExtraEntry> getEntry() const {
        return entry.get();
    }
};

/// \brief Represents a virtual entry in the settings.
///
/// This entry may be connected to a single reference, an entry in \ref Settings object or other user-defined
/// value. It provides an abstraction through which state of jobs can be queried and modified.
class IVirtualEntry : public Polymorphic {
public:
    enum class Type { BOOL, INT, FLOAT, VECTOR, INTERVAL, STRING, PATH, ENUM, EXTRA, FLAGS };

    using Value = Variant<bool, int, Float, Vector, Interval, String, Path, EnumWrapper, ExtraEntry>;

    /// \brief Returns if this entry is currently enabled.
    ///
    /// Implementation may return false if the entry does not have any effect for current setup of the job,
    /// for example if the entry is associated with a parameter of a solver which is not being used.
    virtual bool enabled() const = 0;

    /// \brief Modifies the current value of the entry.
    ///
    /// Function shall throw \ref Exception (or derived type) if the type of the value differs from type of
    /// the entry.
    /// \return True if the value was changed, false if it was rejected.
    virtual bool set(const Value& value) = 0;

    /// \brief Modifies the entry if the value is missing or no reasonable value can be assigned.
    ///
    /// This is used when e.g. loading older session that does not contain newly added entry.
    virtual bool setFromFallback() = 0;

    /// \brief Returns the currently stored value.
    virtual Value get() const = 0;

    /// \brief Checks if the given value is a valid input for this entry.
    virtual bool isValid(const Value& value) const = 0;

    /// \brief Returns the type of this entry.
    virtual Type getType() const = 0;

    /// \brief Returns a descriptive name of the entry.
    ///
    /// This name is intended to be presented to the user (in UI, printed in log, etc.). It may be the same as
    /// the key associated with an entry, although the key is only used as an unique identifier and does not
    /// have to be human-readable.
    virtual String getName() const = 0;

    /// \brief Returns an optional description of the entry.
    virtual String getTooltip() const {
        return "";
    }

    /// \brief Returns true if the entry can modify multiple values simultaneously.
    ///
    /// Usually, there is a 1-1 correspondence between entries and values (stored either in \ref Settings or
    /// directly in the job). However, an entry can modify other values as well, in which case it should
    /// signal this by returning true from this function.
    virtual bool hasSideEffect() const {
        return false;
    }

    enum class PathType {
        DIRECTORY,
        INPUT_FILE,
        OUTPUT_FILE,
    };

    /// \brief Returns the type of the path.
    ///
    /// Used only if type of the entry is PATH, otherwise returns NOTHING.
    virtual Optional<PathType> getPathType() const {
        return NOTHING;
    }

    struct FileFormat {
        String description;
        String extension;
    };

    /// \brief Returns the allowed file format for this file entry.
    ///
    /// Used only for file entry (i.e. getPathType returns INPUT_FILE or OUTPUT_FILE). The returned array
    /// contains pairs of the file format description and the corresponding extension (i.e. 'txt').
    virtual Array<FileFormat> getFileFormats() const {
        return {};
    }
};

/// \brief Helper object, allowing to add units, tooltips and additional properties into the entry created
/// with \ref VirtualSettings::Category::connect.
///
/// Each member function returns a reference to the object in order to allow queuing the calls, such as
/// \code
/// control.setTooltip("This is a tooltip").setUnits(1.e3f);
/// \endcode
///
/// It partially implements the \ref IVirtualEntry interface to avoid code duplication.
class EntryControl : public IVirtualEntry {
public:
    using Enabler = Function<bool()>;
    using Accessor = Function<void(const Value& newValue)>;
    using Validator = Function<bool(const Value& newValue)>;
    using Fallback = Function<void()>;

protected:
    String tooltip;
    Float mult = 1._f;
    Optional<PathType> pathType = NOTHING;
    Array<FileFormat> fileFormats;
    Enabler enabler = nullptr;
    CallbackSet<Accessor> accessors;
    Validator validator = nullptr;
    Fallback fallback = nullptr;
    bool sideEffect = false;

public:
    /// \brief Adds or replaces the previous tooltip associanted with the entry.
    EntryControl& setTooltip(const String& newTooltip);

    /// \brief Sets units in which the entry is stored.
    ///
    /// Note that the units are currently only applied for \ref Float or \ref Vector entries. Other entries
    /// ignore the value.
    EntryControl& setUnits(const Float newMult);

    /// \brief Adds or replaces the enabler functor of the entry.
    ///
    /// Enabler specifies whether the entry is accessible or not. The user can still call the member function
    /// \ref set or \ref get of the entry even if the functor returns false, but it indicates that the entry
    /// has no meaning in the context of the current settings. For example, enabler should returns false for
    /// entries associated with parameters of the boundary if there are no boundary conditions in the
    /// simulations.
    EntryControl& setEnabler(const Enabler& newEnabler);

    /// \brief Sets a function used when no value can be assigned to the entry.
    EntryControl& setFallback(const Fallback& newFallback);

    /// \brief Adds a functor called when the entry changes, i.e. when \ref set function is called.
    ///
    /// There can be any number of accessors set for each property.
    /// \brief owner Owner that registered the accessor. When expired, the accessor is no longer called.
    /// \brief accessor New functor to be added.
    EntryControl& addAccessor(const SharedToken& owner, const Accessor& accessor);

    /// \brief Adds or replaces the functor called to validate the new value.
    ///
    /// If the functor returns false, the value is unchanged.
    EntryControl& setValidator(const Validator& newValidator);

    /// \brief Specifies that the entry has a side effect, i.e. it changes values of other entries.
    EntryControl& setSideEffect();

    /// \brief Sets the type of the path.
    EntryControl& setPathType(const PathType& newType);

    /// \brief Sets the allowed file formats.
    EntryControl& setFileFormats(Array<FileFormat>&& formats);

protected:
    virtual bool enabled() const override final;

    virtual String getTooltip() const override final;

    virtual bool hasSideEffect() const override final;

    virtual Optional<PathType> getPathType() const override final;

    virtual Array<FileFormat> getFileFormats() const override final;

    virtual bool isValid(const Value& value) const override final;

    virtual void setImpl(const Value& value) = 0;

private:
    virtual bool set(const Value& value) override final;

    virtual bool setFromFallback() override final;
};

/// \brief Holds a map of virtual entries, associated with a unique name.
///
/// The key-value pairs are not stored in the settings directly, they are stored in settings categories; the
/// \ref VirtualSettings object then holds a map of categories. This provides clustering of related entries,
/// which helps to separate the entries in UI.
class VirtualSettings {
public:
    class Category {
        friend class VirtualSettings;

    private:
        UnorderedMap<String, AutoPtr<IVirtualEntry>> entries;

    public:
        /// \brief Manually adds a new entry into the settings.
        ///
        /// Mostly intended for custom implementations of \ref IVirtualEntry. For most uses, using \ref
        /// connect is more conventient way to connect values with \ref IVirtualSettings.
        void addEntry(const String& key, AutoPtr<IVirtualEntry>&& entry);

        /// \brief Connects to given reference.
        template <typename TValue>
        EntryControl& connect(const String& name, const String& key, TValue& value);

        /// \brief Connects to value in \ref Settings object
        template <typename TValue, typename TEnum>
        EntryControl& connect(const String& name, Settings<TEnum>& settings, const TEnum id);
    };


private:
    UnorderedMap<String, Category> categories;

public:
    /// \brief Modifies an existing entry in the settings.
    ///
    /// This function cannot be used to add a new entry. Use \ref Category to create entries.
    /// \param key Identifier of the entry
    /// \param value New value of the entry
    /// \throw InvalidSetup if no entry with given key exists.
    void set(const String& key, const IVirtualEntry::Value& value);

    /// \brief Overload allowing to use an ID associated with a \ref Settings entry.
    ///
    /// This is useful if the virtual entry is connected to an entry in a \ref Settings object. This way, an
    /// enum value can be used instead of string, ensuring the value is valid (string may contain typos) and
    /// can be easily renamed if necessary.
    ///
    /// Note that there is no check that the virtual entry is really connected to a "real" entry with given
    /// ID, virtual entry is (intentionally) a black box here.
    /// \param id ID of the connected entry
    /// \param value New value of the entry
    /// \throw InvalidSetup if the ID is invalid or no entry with given ID exists.
    template <typename TEnum, typename = std::enable_if_t<std::is_enum<TEnum>::value>>
    void set(const TEnum id, const IVirtualEntry::Value& value);

    /// \brief Returns a value of an entry.
    ///
    /// \param key Identifier of the entry
    /// \throw InvalidSetup if no entry with given key exists.
    IVirtualEntry::Value get(const String& key) const;

    /// \brief Creates a new category of entries.
    ///
    /// Returned object can be used to add entries into settings.
    /// \param name Name of the created category.
    Category& addCategory(const String& name);

    /// \brief Interface allowing to enumerate all entries in the settings.
    class IEntryProc : public Polymorphic {
    public:
        /// \brief Called for every category in the settings.
        ///
        /// Every subsequent call of \ref onEntry corresponds to an entry belonging to this category.
        /// \param name Name of the category
        virtual void onCategory(const String& name) const = 0;

        /// \brief Called for every entry in the current category.
        ///
        /// \param key Identifier of the entry
        /// \param entry Stored entry
        virtual void onEntry(const String& key, IVirtualEntry& entry) const = 0;
    };

    /// \brief Enumerates all entries stored in the settings.
    void enumerate(const IEntryProc& proc);
};

/// \brief Convenience function, returning the list of input file formats defined by IoEnum.
Array<IVirtualEntry::FileFormat> getInputFormats();

/// \brief Convenience function, returning the list of output file formats defined by IoEnum.
Array<IVirtualEntry::FileFormat> getOutputFormats();

NAMESPACE_SPH_END

#include "run/VirtualSettings.inl.h"
