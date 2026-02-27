#ifndef DSCRIPT_STDLIB_RANDOM_H
#define DSCRIPT_STDLIB_RANDOM_H

#include <string>
#include <unordered_map>

#include "../value.h"

std::unordered_map<std::string, Value> create_random_stdlib();

#endif //DSCRIPT_STDLIB_RANDOM_H
