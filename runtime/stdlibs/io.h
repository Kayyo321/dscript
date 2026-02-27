#ifndef DSCRIPT_STDLIB_IO_H
#define DSCRIPT_STDLIB_IO_H

#include <string>
#include <unordered_map>
#include <vector>

#include "../value.h"

void set_io_program_args(std::vector<std::string> args);
std::unordered_map<std::string, Value> create_io_stdlib();

#endif //DSCRIPT_STDLIB_IO_H
