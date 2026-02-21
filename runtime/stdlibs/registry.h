#ifndef DSCRIPT_STDLIB_REGISTRY_H
#define DSCRIPT_STDLIB_REGISTRY_H

#include <string>
#include <unordered_map>

#include "../value.h"

using StdlibFactory = std::unordered_map<std::string, Value>(*)();

const std::unordered_map<std::string, StdlibFactory> &get_stdlib_registry();

#endif //DSCRIPT_STDLIB_REGISTRY_H
