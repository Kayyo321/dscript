#ifndef DSCRIPT_STDLIB_IO_H
#define DSCRIPT_STDLIB_IO_H

#include <string>
#include <unordered_map>

#include "../value.h"

std::unordered_map<std::string, Value> create_io_stdlib();

#endif //DSCRIPT_STDLIB_IO_H
