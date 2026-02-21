#include "registry.h"

#include "io.h"

const std::unordered_map<std::string, StdlibFactory> &get_stdlib_registry() {
    static const std::unordered_map<std::string, StdlibFactory> registry = {
        {"io", create_io_stdlib},
    };

    return registry;
}
