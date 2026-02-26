//
// Created by sullivanb on 2/20/26.
//

#include "value.h"

#include <cmath>
#include <iomanip>
#include <limits>
#include <ostream>

Value Value::boolean(const bool value) {
    return Value(value);
}

Value Value::number(const double value) {
    return Value(value);
}

Value Value::none() {
    return Value();
}

Value Value::object(const std::shared_ptr<Obj> &value) {
    return Value(value);
}

bool Value::operator==(const Value &other) const {
    if (type != other.type)
        return false;

    switch (type) {
        case ValueType::Boolean:
            return as.boolean == other.as.boolean;

        case ValueType::Number:
            return as.number == other.as.number;

        case ValueType::None:
            return true;

        default:
        case ValueType::Object:
            if (as.object == other.as.object) {
                return true;
            }

            if (as.object == nullptr || other.as.object == nullptr) {
                return false;
            }

            return (*as.object) == (*other.as.object);
    }
}

void Value::print(std::ostream &os) const {
    switch (type) {
        case ValueType::Boolean:
            os << as.boolean;
            break;

        case ValueType::Number: {
            double int_part;

            if (const double fractional_part = std::modf(as.number, &int_part); std::fabs(fractional_part) < std::numeric_limits<double>::epsilon()) {
                os << std::fixed << std::setprecision(0) << as.number;
            } else {
                os << as.number;
            }
            break;
        }

        case ValueType::None:
            os << "none";
            break;

        default:
        case ValueType::Object:
            as.object->print(os);
            break;
    }
}

Value::Value(const bool value) : type{ValueType::Boolean}, as{.boolean = value} {}
Value::Value(const double value) : type{ValueType::Number}, as{.number = value} {}
Value::Value() : type{ValueType::None} {}
Value::Value(const std::shared_ptr<Obj> &value) : type{ValueType::Object}, as{.object = value} {}
