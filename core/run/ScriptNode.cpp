#include "run/ScriptNode.h"
#include "run/ScriptUtils.h"

NAMESPACE_SPH_BEGIN

#ifdef SPH_USE_CHAISCRIPT

namespace Chai {

class Node {
private:
    SharedPtr<JobNode> node;
    VirtualSettings settings;

    const RunSettings& global;
    IJobCallbacks& callbacks;

public:
    Node(SharedPtr<JobNode> node, const RunSettings& global, IJobCallbacks& callbacks)
        : node(node)
        , global(global)
        , callbacks(callbacks) {
        settings = node->getSettings();
    }

    template <typename T>
    void setParam(const std::string& keyUtf, const T value) {
        String key = String::fromUtf8(keyUtf.c_str());
        IVirtualEntry::Value current = settings.get(key);
        if (!current.has<T>()) {
            throw InvalidSetup("Type mismatch when setting parameter '" + key + "'");
        }
        settings.set(key, value);
    }

    void run() {
        if (!callbacks.shouldAbortRun()) {
            node->run(global, callbacks);
        }
    }
};


// either string, Path, or EnumWrapper
template <>
void Node::setParam(const std::string& keyUtf, const std::string valueUtf) {
    String key = String::fromUtf8(keyUtf.c_str());
    String value = String::fromUtf8(valueUtf.c_str());
    IVirtualEntry::Value current = settings.get(key);
    if (current.has<String>()) {
        settings.set(key, value);
    } else if (current.has<Path>()) {
        settings.set(key, Path(value));
    } else if (current.has<EnumWrapper>()) {
        EnumWrapper ew = current.get<EnumWrapper>();
        Optional<int> enumValue = EnumMap::fromString(value, ew.index);
        if (!enumValue) {
            throw InvalidSetup("Unknown value of parameter '{}': {}", key, value);
        }
        ew.value = enumValue.value();
        settings.set(key, ew);
    } else {
        throw InvalidSetup("Type mismatch when setting parameter '{}'", key);
    }
}

} // namespace Chai

void ScriptNode::run(const RunSettings& global, IJobCallbacks& callbacks) {
    chaiscript::ChaiScript chai;
    Chai::registerBindings(chai);

    chai.add(chaiscript::user_type<Chai::Node>(), "Node");
    chai.add(chaiscript::fun(&Chai::Node::setParam<double>), "setParam");
    chai.add(chaiscript::fun(&Chai::Node::setParam<int>), "setParam");
    chai.add(chaiscript::fun(&Chai::Node::setParam<std::string>), "setParam");
    chai.add(chaiscript::fun(&Chai::Node::run), "run");

    chai.add(chaiscript::fun<std::function<Chai::Node(std::string)>>([&](std::string nameUtf) {
        String name = String::fromUtf8(nameUtf.c_str());
        for (const auto& node : nodes) {
            if (node->instanceName() == name) {
                return Chai::Node(node, global, callbacks);
            }
        }
        throw InvalidSetup("Unknown node '" + name + "'");
    }),
        "getNode");

    chai.eval_file(std::string(file.string().toUtf8()));
}

#endif

NAMESPACE_SPH_END
