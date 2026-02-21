#include "objs.h"

#include <ostream>

#include "throwables.h"

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

std::shared_ptr<ObjFunction> ObjFunction::bind(Instance *inst) {
    (void) inst;
    const auto env = std::make_shared<Environment>(closure);
    env->define("self", Value::none());
    return std::make_shared<ObjFunction>(declaration, env, is_init);
}

int ObjFunction::arity() {
    return static_cast<int>(declaration->params.size());
}

Value ObjFunction::call(Vm *vm, const std::vector<std::shared_ptr<Obj>> &args) {
    (void) vm;
    (void) args;

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

ObjNative::ObjNative(const int arity, NativeFn function)
    : Callable(ObjType::Native), arity_value(arity), function(function) {}

int ObjNative::arity() {
    return arity_value;
}

Value ObjNative::call(Vm *vm, const std::vector<std::shared_ptr<Obj>> &args) {
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

Value ObjClass::call(Vm *vm, const std::vector<std::shared_ptr<Obj>> &args) {
    (void) vm;
    (void) args;

    const auto instance = std::make_shared<ObjInstance>(std::make_shared<ObjClass>(*this));
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

ObjInstance::ObjInstance(std::shared_ptr<ObjClass> klass)
    : Obj(ObjType::Instance), klass(std::move(klass)) {}

bool ObjInstance::operator==(const Obj &other) {
    return this == &other;
}

void ObjInstance::print(std::ostream &os) {
    os << klass->name << " instance";
}

Value ObjInstance::get(const std::string &field_name) {
    const auto field_it = fields.find(field_name);
    if (field_it != fields.end()) {
        return field_it->second;
    }

    const auto method = klass->find_method(field_name);
    if (method != nullptr) {
        return Value::object(method);
    }

    throw RuntimeError("Undefined property '" + field_name + "'.");
}

void ObjInstance::set(const std::string &field_name, Value value) {
    fields.insert_or_assign(field_name, std::move(value));
}
