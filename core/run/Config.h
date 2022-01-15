#pragma once

/// \file Config.h
/// \brief Interface for the configuration files storing job data.
/// \author Pavel Sevecek (sevecek at sirrah.troja.mff.cuni.cz)
/// \date 2016-2021

#include "io/Path.h"
#include "objects/Exceptions.h"
#include "objects/containers/UnorderedMap.h"
#include "objects/geometry/Vector.h"
#include "objects/wrappers/Function.h"

NAMESPACE_SPH_BEGIN

class ITextInputStream;

using ConfigException = Exception;

/// \brief Interface for value written to the config file.
///
/// It provides a way to serialize and deserialize the value from string. This class should not be implemented
/// directly, instead new types should be added by specialization of class \ref ConfigValue; \ref IConfigValue
/// serves only as type erasure, but to access the stored value, it is necessary to cast it to \ref
/// ConfigValue.
class IConfigValue : public Polymorphic {
public:
    /// \brief Writes the value into a string.
    virtual String write() const = 0;

    /// \brief Reads the value from string and stores it internally.
    ///
    /// \throw ConfigException if the value cannot be deserialized.
    virtual void read(const String& source) = 0;
};

/// \brief Helper function wrapping a string by quotes.
String quoted(const String& value);

/// \brief Removes leading and trailing quote from a string.
String unquoted(const String& value);

/// \brief Generic implementation of \ref IConfigValue, using std::wstringstream for the (de)serialization.
///
/// This is used for primitive types and for other types that define the stream operators. If a user-defined
/// type does not have these operators, it is necessary to specialize this class and implement the functions
/// \ref read and \ref write manually.
template <typename Type>
class ConfigValue : public IConfigValue {
private:
    Type value;

public:
    ConfigValue() = default;

    ConfigValue(const Type& value)
        : value(value) {}

    virtual String write() const override {
        return toString(value);
    }

    virtual void read(const String& source) override {
        if (Optional<Type> v = fromString<Type>(source)) {
            value = v.value();
        } else {
            throw ConfigException("Invalid value '" + source + "'");
        }
    }

    Type get() const {
        return value;
    }
};

/// \copydoc ConfigValue.
///
/// Specialization for \ref Vector.
template <>
class ConfigValue<Vector> : public IConfigValue {
private:
    Vector value;

public:
    ConfigValue() = default;

    ConfigValue(const Vector& value)
        : value(value) {}

    virtual String write() const override {
        return toString(value);
    }

    virtual void read(const String& source) override {
        std::wstringstream ss(source.toUnicode());
        ss >> value[X] >> value[Y] >> value[Z];
    }

    Vector get() const {
        return value;
    }
};

/// \copydoc ConfigValue.
///
/// Specialization for \ref Interval.
template <>
class ConfigValue<Interval> : public IConfigValue {
private:
    Interval value;

public:
    ConfigValue() = default;

    ConfigValue(const Interval& value)
        : value(value) {}

    virtual String write() const override {
        return toString(value);
    }

    virtual void read(const String& source) override {
        std::wstringstream ss(source.toUnicode());
        Float lower, upper;
        ss >> lower >> upper;
        value = Interval(lower, upper);
    }

    Interval get() const {
        return value;
    }
};

/// \copydoc ConfigValue.
///
/// Specialization for \ref String.
template <>
class ConfigValue<String> : public IConfigValue {
private:
    String value;

public:
    ConfigValue() = default;

    ConfigValue(const String& value)
        : value(value) {}

    virtual String write() const override {
        return quoted(value);
    }

    virtual void read(const String& source) override {
        value = unquoted(source);
    }

    const String& get() const {
        return value;
    }
};

/// \copydoc ConfigValue.
///
/// Specialization for \ref Path.
template <>
class ConfigValue<Path> : public IConfigValue {
private:
    Path value;

public:
    ConfigValue() = default;

    ConfigValue(const Path& value)
        : value(value) {}

    virtual String write() const override {
        return quoted(value.string());
    }

    virtual void read(const String& source) override {
        value = Path(unquoted(source));
    }

    Path get() const {
        return value;
    }
};

/// \brief Represents a single node in the hierarchy written into config file.
///
/// Each node can store an arbitrary number of entries (as key-value pairs) and also a number of child nodes.
/// Values are internally stored as strings, so it is necessary to specify the type of the value to read it.
class ConfigNode {
    friend class Config;

private:
    /// All child nodes
    UnorderedMap<String, SharedPtr<ConfigNode>> children;

    /// All entries in this node
    UnorderedMap<String, String> entries;

public:
    /// \brief Adds a new value into the node.
    template <typename Type>
    void set(const String& name, const Type& value) {
        ConfigValue<Type> writer(value);
        entries.insert(name, writer.write());
    }

    /// \brief Returns a value stored in the node.
    ///
    /// \throw ConfigException if the value does not exist or cannot be deserialized.
    template <typename Type>
    Type get(const String& name) const {
        Optional<Type> opt = this->tryGet<Type>(name);
        if (!opt) {
            throw ConfigException("Entry '" + name + "' not in config");
        }
        return opt.value();
    }

    /// \brief Tries to return a value stored in the node.
    ///
    /// If the value does not exist or cannot be deserialized, the function returns NOTHING.
    template <typename Type>
    Optional<Type> tryGet(const String& name) const {
        auto value = entries.tryGet(name);
        if (!value) {
            return NOTHING;
        }
        ConfigValue<Type> reader;
        reader.read(value.value());
        return reader.get();
    }

    /// \brief Checks if the node contains an entry of given name.
    bool contains(const String& name) const {
        return entries.contains(name);
    }

    /// \brief Returns the number of entries stored in the node.
    Size size() const;

    /// \brief Adds a new child node to this node.
    SharedPtr<ConfigNode> addChild(const String& name);

    /// \brief Returns a child node.
    ///
    /// \throw ConfigException if no such node exists.
    SharedPtr<ConfigNode> getChild(const String& name);

    /// \brief Calls the provided functor for each child node.
    void enumerateChildren(Function<void(String name, ConfigNode& node)> func);

private:
    /// \brief Serializes the content of this node to a string stream.
    void write(const String& padding, std::wstringstream& source);

    /// \brief Deserializes the node from a string stream.
    void read(ITextInputStream& source);
};

/// \brief Provides functionality for reading and writing configuration files.
///
/// Configuration files consist of key-value pair, clustered in a node hierarchy.
class Config {
private:
    UnorderedMap<String, SharedPtr<ConfigNode>> nodes;

public:
    /// \brief Adds a new node to the config.
    SharedPtr<ConfigNode> addNode(const String& name);

    /// \brief Returns a node with given name.
    ///
    /// \throw ConfigException if no such node exists.
    SharedPtr<ConfigNode> getNode(const String& name);

    /// \brief Returns a node with given name or nullptr if no such node exists.
    SharedPtr<ConfigNode> tryGetNode(const String& name);

    /// \brief Deserializes the input string stream into nodes.
    ///
    /// This removed all previously added nodes in the config.
    /// \throw ConfigException if the source has invalid format.
    void read(ITextInputStream& source);

    /// \brief Serializes all nodes in the config into a string.
    String write();

    /// \brief Reads content of given file and deserializes the config from the loaded string.
    void load(const Path& path);

    /// \brief Serializes all nodes in the config into a file.
    void save(const Path& path);

    /// \brief Calls the provided functor for all nodes in the config.
    void enumerate(Function<void(String, ConfigNode&)> func);
};


NAMESPACE_SPH_END
