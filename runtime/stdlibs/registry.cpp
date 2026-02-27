#include "registry.h"

#include "core.h"
#include "io.h"
#include "random.h"

const std::unordered_map<std::string, StdlibFactory> &get_stdlib_registry() {
    static const std::unordered_map<std::string, StdlibFactory> registry = {
        {"core", create_core_stdlib},
        {"io", create_io_stdlib},
        {"random", create_random_stdlib},
    };

    return registry;
}
