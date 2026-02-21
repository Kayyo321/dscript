#ifndef THROABLES_H
#define THROABLES_H

#include <optional>
#include <stdexcept>
#include <string>

#include "value.h"
#include "../lexing/token.h"

enum class RuntimeErrorKind {
    Runtime,
    Type,
    Name,
    Call,
    Arity,
    Property,
    Index,
};

class RuntimeError : public std::runtime_error {
public:
    explicit RuntimeError(const std::string &message)
        : std::runtime_error(message), kind(RuntimeErrorKind::Runtime), location(std::nullopt) {}

    RuntimeError(RuntimeErrorKind kind, const std::string &message)
        : std::runtime_error(message), kind(kind), location(std::nullopt) {}

    RuntimeError(RuntimeErrorKind kind, const std::string &message, const FilePos &location)
        : std::runtime_error(message), kind(kind), location(location) {}

    RuntimeErrorKind kind;
    std::optional<FilePos> location;
};

class TypeError : public RuntimeError {
public:
    explicit TypeError(const std::string &message)
        : RuntimeError(RuntimeErrorKind::Type, message) {}

    TypeError(const std::string &message, const FilePos &location)
        : RuntimeError(RuntimeErrorKind::Type, message, location) {}
};

class NameError : public RuntimeError {
public:
    explicit NameError(const std::string &message)
        : RuntimeError(RuntimeErrorKind::Name, message) {}

    NameError(const std::string &message, const FilePos &location)
        : RuntimeError(RuntimeErrorKind::Name, message, location) {}
};

class CallError : public RuntimeError {
public:
    explicit CallError(const std::string &message)
        : RuntimeError(RuntimeErrorKind::Call, message) {}

    CallError(const std::string &message, const FilePos &location)
        : RuntimeError(RuntimeErrorKind::Call, message, location) {}
};

class ArityError : public RuntimeError {
public:
    explicit ArityError(const std::string &message)
        : RuntimeError(RuntimeErrorKind::Arity, message) {}

    ArityError(const std::string &message, const FilePos &location)
        : RuntimeError(RuntimeErrorKind::Arity, message, location) {}
};

class PropertyError : public RuntimeError {
public:
    explicit PropertyError(const std::string &message)
        : RuntimeError(RuntimeErrorKind::Property, message) {}

    PropertyError(const std::string &message, const FilePos &location)
        : RuntimeError(RuntimeErrorKind::Property, message, location) {}
};

class IndexError : public RuntimeError {
public:
    explicit IndexError(const std::string &message)
        : RuntimeError(RuntimeErrorKind::Index, message) {}

    IndexError(const std::string &message, const FilePos &location)
        : RuntimeError(RuntimeErrorKind::Index, message, location) {}
};

class Return : public std::runtime_error {
public:
    explicit Return(Value value)
        : std::runtime_error("return"), value(std::move(value)) {}

    Value value;
};

#endif //THROABLES_H