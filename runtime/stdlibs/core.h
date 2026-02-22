#ifndef DSCRIPT_STDLIB_CORE_H
#define DSCRIPT_STDLIB_CORE_H

#include <string>
#include <unordered_map>

#include "../value.h"

std::unordered_map<std::string, Value> create_core_stdlib();

#endif //DSCRIPT_STDLIB_CORE_H
