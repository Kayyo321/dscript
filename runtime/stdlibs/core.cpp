#include "core.h"

#include <sstream>
#include <string>

#include "../objs.h"
#include "../throwables.h"

static Value native_error(const std::vector<Value> &args) {
    if (args.size() != 1) {
        throw ArityError("error() expects exactly one argument.");
    }

    const Value &message = args[0];
    if (message.type != ValueType::Object || message.as.object->get_type() != ObjType::String) {
        throw TypeError("error() argument must be a string.");
    }

    const auto message_obj = std::static_pointer_cast<ObjString>(message.as.object);
    return Value::object(std::make_shared<ObjError>(message_obj->chars));
}

static Value native_length(const std::vector<Value> &args) {
    if (args.size() != 1) {
        throw ArityError("length() expects exactly one argument.");
    }

    const Value &value = args[0];
    if (value.type != ValueType::Object) {
        throw TypeError("length() expects a string or list.");
    }

    if (value.as.object->get_type() == ObjType::String) {
        const auto str = std::static_pointer_cast<ObjString>(value.as.object);
        return Value::number(static_cast<double>(str->chars.size()));
    }

    if (value.as.object->get_type() == ObjType::List) {
        const auto list = std::static_pointer_cast<List>(value.as.object);
        return Value::number(static_cast<double>(list->elements.size()));
    }

    throw TypeError("length() expects a string or list.");
}

static Value native_append(const std::vector<Value> &args) {
    if (args.size() != 2) {
        throw ArityError("append() expects exactly two arguments.");
    }

    const Value &target = args[0];
    const Value &item = args[1];

    if (target.type != ValueType::Object) {
        throw TypeError("append() target must be a list or string.");
    }

    if (target.as.object->get_type() == ObjType::List) {
        const auto list = std::static_pointer_cast<List>(target.as.object);
        list->elements.push_back(item);
        return target;
    }

    if (target.as.object->get_type() == ObjType::String) {
        if (item.type != ValueType::Object || item.as.object->get_type() != ObjType::String) {
            throw TypeError("append() on string requires a string value.");
        }

        const auto left = std::static_pointer_cast<ObjString>(target.as.object);
        const auto right = std::static_pointer_cast<ObjString>(item.as.object);
        return Value::object(std::make_shared<ObjString>(left->chars + right->chars));
    }

    throw TypeError("append() target must be a list or string.");
}

static Value native_to_string(const std::vector<Value> &args) {
    if (args.size() != 1) {
        throw ArityError("to_string() expects exactly one argument.");
    }

    std::ostringstream oss;
    args[0].print(oss);
    return Value::object(std::make_shared<ObjString>(oss.str()));
}

static Value native_to_number(const std::vector<Value> &args) {
    if (args.size() != 1) {
        throw ArityError("to_number() expects exactly one argument.");
    }

    const Value &value = args[0];
    if (value.type == ValueType::Number) {
        return value;
    }

    if (value.type == ValueType::Boolean) {
        return Value::number(value.as.boolean ? 1.0 : 0.0);
    }

    if (value.type != ValueType::Object || value.as.object->get_type() != ObjType::String) {
        throw TypeError("to_number() expects a number, boolean, or numeric string.");
    }

    const auto str = std::static_pointer_cast<ObjString>(value.as.object);
    const std::string &text = str->chars;

    try {
        std::size_t consumed = 0;
        const double num = std::stod(text, &consumed);
        if (consumed != text.size()) {
            throw TypeError("to_number() string contains non-numeric characters.");
        }
        return Value::number(num);
    } catch (const std::invalid_argument &) {
        throw TypeError("to_number() string is not numeric.");
    } catch (const std::out_of_range &) {
        throw TypeError("to_number() string is out of numeric range.");
    }
}

std::unordered_map<std::string, Value> create_core_stdlib() {
    std::unordered_map<std::string, Value> exports;
    exports.insert_or_assign("error", Value::object(std::make_shared<ObjNative>(1, native_error)));
    exports.insert_or_assign("length", Value::object(std::make_shared<ObjNative>(1, native_length)));
    exports.insert_or_assign("append", Value::object(std::make_shared<ObjNative>(2, native_append)));
    exports.insert_or_assign("to_string", Value::object(std::make_shared<ObjNative>(1, native_to_string)));
    exports.insert_or_assign("to_number", Value::object(std::make_shared<ObjNative>(1, native_to_number)));
    return exports;
}
