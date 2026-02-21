#ifndef KLASS_H
#define KLASS_H

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <iosfwd>

#include "objs.h"
#include "value.h"
#include "instance.h"

class Class: public Callable {
public:
    explicit Class(std::string name, std::map<std::string, std::shared_ptr<ObjFunction>> methods)
        : Callable(ObjType::Class), name(std::move(name)), methods(std::move(methods)) {}

    std::shared_ptr<ObjFunction> find_method(const std::string &name) {
        if (methods.find(name) != methods.end()) {
            return methods[name];
        }

        if (super_class != nullptr) {
            return super_class->find_method(name);
        }

        return nullptr;
    }

    int arity() override {
        const std::shared_ptr<ObjFunction> init = find_method("init");
        if (init == nullptr) {
            return 0;
        }
        return init->arity();
    }

    Value call(Vm *vm, const std::vector<Value> &args) override {
        (void) args;
        std::shared_ptr<ObjFunction> init = find_method("init");
        if (init != nullptr) {
            init->call(vm, {});
        }
        return Value::none();
    }

    bool operator==(const Obj &other) override {
        return this == &other;
    }

    void print(std::ostream &os) override {
        os << name;
    }

    std::string name;
    std::shared_ptr<Class> super_class;

private:
    std::map<std::string, std::shared_ptr<ObjFunction>> methods;
};

#endif //KLASS_H