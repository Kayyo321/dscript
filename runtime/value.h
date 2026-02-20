//
// Created by sullivanb on 2/20/26.
//

#ifndef DSCRIPT_VALUE_H
#define DSCRIPT_VALUE_H

#include "obj.h"

#include <memory>

enum class ValueType {
    Boolean,
    Number,
    None,
    Object,
};

struct Value {
    static Value boolean(bool value);
    static Value number(double value);
    static Value none();
    static Value object(const std::shared_ptr<Obj> &value);

    ValueType type;
    struct {
        bool boolean{false};
        double number{0.0};
        std::shared_ptr<Obj> object{nullptr};
    } as;

    bool operator==(const Value &other) const;
    void print(std::ostream &os) const;

private:
    explicit Value(bool value);
    explicit Value(double value);
    Value();
    explicit Value(const std::shared_ptr<Obj> &value);
};

#endif //DSCRIPT_VALUE_H