#include "objs.h"

#include <cmath>
#include <ostream>

#include "throwables.h"
#include "vm.h"

ObjString::ObjString(std::string chars)
    : Obj(ObjType::String), chars(std::move(chars)) {}

bool ObjString::operator==(const Obj &other) {
    if (other.get_type() != ObjType::String) {
        return false;
    }
    return chars == static_cast<const ObjString &>(other).chars;
}

void ObjString::print(std::ostream &os) {
    os << chars;
}

Value ObjString::index(const Value &key) {
    if (key.type != ValueType::Number) {
        throw TypeError("String index must be a number.");
    }

    const double index_d = key.as.number;
    if (std::floor(index_d) != index_d) {
        throw IndexError("String index must be an integer.");
    }

    const int index = static_cast<int>(index_d);
    if (index < 0 || index >= static_cast<int>(chars.size())) {
        throw IndexError("String index out of bounds.");
    }

    return Value::object(std::make_shared<ObjString>(std::string(1, chars[index])));
}

std::shared_ptr<ObjFunction> ObjFunction::bind(ObjInstance *inst) {
    const auto env = std::make_shared<Environment>(closure);
    env->define("self", Value::object(std::shared_ptr<ObjInstance>(inst, [](ObjInstance *) {})));
    return std::make_shared<ObjFunction>(declaration, env, is_init);
}

int ObjFunction::arity() {
    const int param_count = static_cast<int>(declaration->params.size());
    const int block_count = declaration->blocks.has_value() ? static_cast<int>(declaration->blocks->size()) : 0;
    return param_count + block_count;
}

Value ObjFunction::call(Vm *vm, const std::vector<Value> &args) {
    const auto env = std::make_shared<Environment>(closure);

    std::size_t arg_idx = 0;
    for (const auto &param : declaration->params) {
        env->define(param.literal.lexeme, args[arg_idx++]);
    }

    if (declaration->blocks.has_value()) {
        for (const auto &block : declaration->blocks.value()) {
            const Value block_value = args[arg_idx++];
            env->define(block.name.literal.lexeme, block_value);
            if (block.expect.has_value()) {
                env->define(block.expect->literal.lexeme, block_value);
            }
        }
    }

    try {
        vm->execute_block(declaration->body, env);
    } catch (const Return &r) {
        if (is_init) {
            return closure->get_at(0, "self");
        }
        return r.value;
    }

    if (is_init) {
        return closure->get_at(0, "self");
    }

    return Value::none();
}

bool ObjFunction::operator==(const Obj &other) {
    if (other.get_type() != ObjType::Function) {
        return false;
    }

    const auto &function = static_cast<const ObjFunction &>(other);
    return declaration == function.declaration && closure == function.closure && is_init == function.is_init;
}

void ObjFunction::print(std::ostream &os) {
    os << "<fn " << declaration->name.literal.lexeme << ">";
}

Value ObjFunction::index(const Value &key) {
    if (key.type != ValueType::Object || key.as.object->get_type() != ObjType::String) {
        throw TypeError("Function index key must be a string.");
    }

    const auto key_string = std::static_pointer_cast<ObjString>(key.as.object);
    if (key_string->chars == "name") {
        return Value::object(std::make_shared<ObjString>(declaration->name.literal.lexeme));
    }

    if (key_string->chars == "arity") {
        return Value::number(arity());
    }

    if (key_string->chars == "is_init") {
        return Value::boolean(is_init);
    }

    throw IndexError("Unknown function index key '" + key_string->chars + "'.");
}

ObjNative::ObjNative(const int arity, NativeFn function)
    : Callable(ObjType::Native), arity_value(arity), function(function) {}

int ObjNative::arity() {
    return arity_value;
}

Value ObjNative::call(Vm *vm, const std::vector<Value> &args) {
    (void) vm;
    return function(args);
}

bool ObjNative::operator==(const Obj &other) {
    if (other.get_type() != ObjType::Native) {
        return false;
    }
    return function == static_cast<const ObjNative &>(other).function;
}

void ObjNative::print(std::ostream &os) {
    os << "<native fn>";
}

Value ObjNative::index(const Value &key) {
    if (key.type != ValueType::Object || key.as.object->get_type() != ObjType::String) {
        throw TypeError("Native function index key must be a string.");
    }

    const auto key_string = std::static_pointer_cast<ObjString>(key.as.object);
    if (key_string->chars == "arity") {
        return Value::number(arity());
    }

    throw IndexError("Unknown native function index key '" + key_string->chars + "'.");
}

ObjClass::ObjClass(std::string name, std::map<std::string, std::shared_ptr<ObjFunction>> methods)
    : Callable(ObjType::Class), name(std::move(name)), methods(std::move(methods)) {}

std::shared_ptr<ObjFunction> ObjClass::find_method(const std::string &method_name) const {
    const auto it = methods.find(method_name);
    if (it == methods.end()) {
        return nullptr;
    }
    return it->second;
}

int ObjClass::arity() {
    const auto init = find_method("init");
    if (init == nullptr) {
        return 0;
    }
    return init->arity();
}

Value ObjClass::call(Vm *vm, const std::vector<Value> &args) {
    const auto instance = std::make_shared<ObjInstance>(std::make_shared<ObjClass>(*this));
    const auto init = find_method("init");
    if (init != nullptr) {
        init->bind(instance.get())->call(vm, args);
    }
    return Value::object(instance);
}

bool ObjClass::operator==(const Obj &other) {
    if (other.get_type() != ObjType::Class) {
        return false;
    }

    const auto &klass = static_cast<const ObjClass &>(other);
    return name == klass.name;
}

void ObjClass::print(std::ostream &os) {
    os << name;
}

Value ObjClass::index(const Value &key) {
    if (key.type != ValueType::Object || key.as.object->get_type() != ObjType::String) {
        throw TypeError("Class index key must be a string.");
    }

    const auto key_string = std::static_pointer_cast<ObjString>(key.as.object);
    const auto method = find_method(key_string->chars);
    if (method != nullptr) {
        return Value::object(method);
    }

    throw PropertyError("Undefined class method '" + key_string->chars + "'.");
}

ObjInstance::ObjInstance(std::shared_ptr<ObjClass> klass)
    : Obj(ObjType::Instance), klass(std::move(klass)) {}

bool ObjInstance::operator==(const Obj &other) {
    return this == &other;
}

void ObjInstance::print(std::ostream &os) {
    os << klass->name << " instance";
}

Value ObjInstance::index(const Value &key) {
    if (key.type != ValueType::Object || key.as.object->get_type() != ObjType::String) {
        throw TypeError("Instance index key must be a string.");
    }

    const auto key_string = std::static_pointer_cast<ObjString>(key.as.object);
    return get(key_string->chars);
}

Value ObjInstance::get(const std::string &field_name) {
    const auto field_it = fields.find(field_name);
    if (field_it != fields.end()) {
        return field_it->second;
    }

    const auto method = klass->find_method(field_name);
    if (method != nullptr) {
        return Value::object(method->bind(this));
    }

    throw PropertyError("Undefined property '" + field_name + "'.");
}

void ObjInstance::set(const std::string &field_name, Value value) {
    fields.insert_or_assign(field_name, std::move(value));
}

List::List(std::vector<Value> elements)
    : Obj(ObjType::List), elements(std::move(elements)) {}

bool List::operator==(const Obj &other) {
    if (other.get_type() != ObjType::List) {
        return false;
    }

    const auto &list = static_cast<const List &>(other);
    return elements == list.elements;
}

void List::print(std::ostream &os) {
    os << "[";
    for (size_t i = 0; i < elements.size(); ++i) {
        elements[i].print(os);
        if (i < elements.size() - 1) {
            os << ", ";
        }
    }
    os << "]";
}

Value List::index(const Value &key) {
    if (key.type != ValueType::Number) {
        throw TypeError("List index must be a number.");
    }

    const double index_d = key.as.number;
    if (std::floor(index_d) != index_d) {
        throw IndexError("List index must be an integer.");
    }

    const int index = static_cast<int>(index_d);
    if (index < 0 || index >= static_cast<int>(elements.size())) {
        throw IndexError("List index out of bounds.");
    }

    return elements[index];
}

ObjError::ObjError(std::string message)
    : Obj(ObjType::Error), message(std::move(message)) {}

bool ObjError::operator==(const Obj &other) {
    if (other.get_type() != ObjType::Error) {
        return false;
    }

    const auto &error = static_cast<const ObjError &>(other);
    return message == error.message;
}

void ObjError::print(std::ostream &os) {
    os << "error(" << message << ")";
}

Value ObjError::index(const Value &key) {
    if (key.type != ValueType::Object || key.as.object->get_type() != ObjType::String) {
        throw TypeError("Error index key must be a string.");
    }

    const auto key_string = std::static_pointer_cast<ObjString>(key.as.object);
    if (key_string->chars == "message") {
        return Value::object(std::make_shared<ObjString>(message));
    }

    throw IndexError("Unknown error index key '" + key_string->chars + "'.");
}
