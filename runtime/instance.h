//
// Created by sullivanb on 2/21/26.
//

#ifndef DSCRIPT_INSTANCE_H
#define DSCRIPT_INSTANCE_H

#include <map>
#include <memory>
#include <string>

#include "../lexing/token.h"
#include "klass.h"
#include "value.h"
#include "objs.h"
#include "throwables.h"

class Instance {
public:
    explicit Instance(std::shared_ptr<Class> klass) 
        : klass(std::move(klass)) {}

    Value get(const Token &name) {
        if (fields.find(name.literal.lexeme) != fields.end()) {
            return fields[name.literal.lexeme];
        } 
        
        std::shared_ptr<ObjFunction> method = klass->find_method(name.literal.lexeme);
        if (method != nullptr) {
            return Value::object(method->bind(this));
        }

        throw RuntimeError("Undefined property '" + name.literal.lexeme + "'.");
    }

    Value set(const Token &name, Value value) {
        fields.insert_or_assign(name.literal.lexeme, std::move(value));
        return value;
    }

    std::shared_ptr<Class> klass;

private:
    std::map<std::string, Value> fields;
};

#endif //DSCRIPT_INSTANCE_H