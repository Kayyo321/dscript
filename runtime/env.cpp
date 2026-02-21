#include "env.h"

#include "throwables.h"

Value Environment::get(const std::string &name) {
    const auto local = values.find(name);
    if (local != values.end()) {
        return local->second;
    } else if (enclosing != nullptr) {
        return enclosing->get(name);
    } else {
        throw NameError("Undefined variable '" + name + "'.");
    }
}

Value Environment::get_at(int distance, const std::string &name) {
    auto env = ancestor(distance);
    const auto it = env->values.find(name);
    if (it == env->values.end()) {
        throw NameError("Undefined variable '" + name + "'.");
    }
    return it->second;
}

void Environment::define(const std::string &name, Value value) {
    values.insert_or_assign(name, std::move(value));
}

void Environment::assign(const std::string &name, Value value) {
    const auto local = values.find(name);
    if (local != values.end()) {
        local->second = std::move(value);
    } else if (enclosing != nullptr) {
        enclosing->assign(name, std::move(value));
    } else {
        throw NameError("Undefined variable '" + name + "'.");
    }
}

void Environment::assign_at(int distance, const std::string &name, Value value) {
    ancestor(distance)->values.insert_or_assign(name, std::move(value));
}

std::shared_ptr<Environment> Environment::ancestor(int distance) {
    std::shared_ptr<Environment> environment = shared_from_this();
    for (int i = 0; i < distance; ++i) {
        environment = environment->enclosing;
    }
    return environment;
}