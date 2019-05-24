#pragma once

#include "io/Path.h"
#include "objects/containers/UnorderedMap.h"
#include "objects/wrappers/Function.h"
#include "system/Settings.h"

NAMESPACE_SPH_BEGIN

/// \brief Represents a virtual entry in the settings.
///
/// This entry may be connected to a single reference, an entry in \ref Settings object or other user-defined
/// value. It provides an abstraction through which state of workers can be queried and modified.
class IVirtualEntry : public Polymorphic {
public:
    enum class Type { BOOL, INT, FLOAT, VECTOR, STRING, PATH, ENUM, FLAGS };

    using Value = Variant<bool, int, Float, Vector, std::string, Path, EnumWrapper>;

    /// \brief Returns if this entry is currently enabled.
    ///
    /// Implementation may return false if the entry does not have any effect for current setup of the worker,
    /// for example if the entry is associated with a parameter of a solver which is not being used.
    virtual bool enabled() const = 0;

    /// \brief Modifies the current value of the entry.
    ///
    /// Function shall throw \ref Exception (or derived type) if the type of the value differs from type of
    /// the entry.
    virtual void set(const Value& value) = 0;

    /// \brief Returns the currently stored value.
    virtual Value get() const = 0;

    /// \brief Returns the type of this entry.
    virtual Type getType() const = 0;

    /// \brief Returns a descriptive name of the entry.
    ///
    /// This name is intended to be presented to the user (in UI, printed in log, etc.). It may be the same as
    /// the key associated with an entry, although the key is only used as an unique identifier and does not
    /// have to be human-readable.
    virtual std::string getName() const = 0;

    /// \brief Returns an optional description of the entry.
    virtual std::string getTooltip() const {
        return "";
    }
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
        UnorderedMap<std::string, AutoPtr<IVirtualEntry>> entries;

    public:
        void addEntry(const std::string& key, AutoPtr<IVirtualEntry>&& entry);

        /// \brief Connects to given reference.
        template <typename TValue>
        Category& connect(const std::string& name,
            const std::string& key,
            TValue& value,
            const Float mult = 1._f,
            const std::string& tooltip = "");

        template <typename TValue>
        Category& connect(const std::string& name,
            const std::string& key,
            TValue& value,
            Function<bool()> enabler,
            const std::string& tooltip = "");

        template <typename TValue>
        Category& connect(const std::string& name,
            const std::string& key,
            TValue& value,
            const std::string& tooltip);

        /// \brief Connects to value in \ref Settings object
        template <typename TValue, typename TEnum>
        Category& connect(const std::string& name,
            Settings<TEnum>& settings,
            const TEnum id,
            const Float mult = 1._f);

        template <typename TValue, typename TEnum>
        Category& connect(const std::string& name,
            Settings<TEnum>& settings,
            const TEnum id,
            const std::string& tooltip);

        template <typename TValue, typename TEnum>
        Category& connect(const std::string& name,
            Settings<TEnum>& settings,
            const TEnum id,
            Function<bool()> enabler,
            const Float mult = 1._f);

        template <typename TValue, typename TEnum>
        Category& connect(const std::string& name,
            Settings<TEnum>& settings,
            const TEnum id,
            Function<bool()> enabler,
            const Float mult,
            const std::string& tooltip);
    };


private:
    UnorderedMap<std::string, Category> categories;

public:
    /// \brief Modifies an existing entry in the settings.
    ///
    /// This function cannot be used to add a new entry. Use \ref Category to create entries.
    /// \param key Identifier of the entry
    /// \param value New value of the entry
    /// \throw InvalidSetup if no entry with given key exists.
    void set(const std::string& key, const IVirtualEntry::Value& value);

    /// \brief Returns a value of an entry.
    ///
    /// \param key Identifier of the entry
    /// \throw InvalidSetup if no entry with given key exists.
    IVirtualEntry::Value get(const std::string& key) const;

    /// \brief Creates a new category of entries.
    ///
    /// Returned object can be used to add entries into settings.
    /// \param name Name of the created category.
    Category& addCategory(const std::string& name);

    /// \brief Interface allowing to enumerate all entries in the settings.
    class IEntryProc : public Polymorphic {
    public:
        /// \brief Called for every category in the settings.
        ///
        /// Every subsequent call of \ref onEntry corresponds to an entry belonging to this category.
        /// \param name Name of the category
        virtual void onCategory(const std::string& name) const = 0;

        /// \brief Called for every entry in the current category.
        ///
        /// \param key Identifier of the entry
        /// \param entry Stored entry
        virtual void onEntry(const std::string& key, IVirtualEntry& entry) const = 0;
    };

    /// \brief Enumerates all entries stored in the settings.
    void enumerate(const IEntryProc& proc);
};

NAMESPACE_SPH_END

#include "run/VirtualSettings.inl.h"
