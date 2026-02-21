//
// Created by sullivanb on 2/21/26.
//

#ifndef DSCRIPT_ENV_H
#define DSCRIPT_ENV_H

#include <map>
#include <string>
#include <memory>

#include "value.h"

class Environment {
public:
    Environment() = default;

    explicit Environment(std::shared_ptr<Environment> enclosing) 
        : enclosing(std::move(enclosing)) {}

    Value get(const std::string &name);
    Value get_at(int distance, const std::string &name);
    void define(const std::string &name, Value value);
    void assign(const std::string &name, Value value);
    void assign_at(int distance, const std::string &name, Value value);
    std::shared_ptr<Environment> ancestor(int distance);

private:
    std::shared_ptr<Environment> shared_from_this() {
        return std::shared_ptr<Environment>(this, [](Environment *) {});
    }

    std::map<std::string, Value> values;
    std::shared_ptr<Environment> enclosing{nullptr};
};

#endif //DSCRIPT_ENV_H