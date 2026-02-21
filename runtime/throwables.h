#ifndef THROABLES_H
#define THROABLES_H

#include <stdexcept>
#include <string>

#include "value.h"

class RuntimeError : public std::runtime_error {
public:
    explicit RuntimeError(const std::string &message)
        : std::runtime_error(message) {}
};

class Return : public std::runtime_error {
public:
    explicit Return(Value value)
        : std::runtime_error("return"), value(std::move(value)) {}

    Value value;
};

#endif //THROABLES_H