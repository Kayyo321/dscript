//
// Created by sullivanb on 2/20/26.
//

#ifndef DSCRIPT_OBJS_H
#define DSCRIPT_OBJS_H

#include <map>

#include <iosfwd>

#include "obj.h"

#include "value.h"
#include "env.h"

#include <string>
#include <map>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>
#include <memory>

#include "../parsing/stmt.h"

class Vm;
class ObjInstance;

class ObjString : public Obj {
public:
    explicit ObjString(std::string chars);

    bool operator==(const Obj &other) override;
    void print(std::ostream &os) override;
    Value index(const Value &key) override;

    std::string chars;
};

class Callable: public Obj {
protected:
    explicit Callable(const ObjType type) : Obj(type) {}

public:
    virtual ~Callable() = default;

    virtual int arity() { return 0; }

    virtual Value call(Vm *vm, const std::vector<Value> &args) = 0;
};

class ObjFunction: public Callable {
public:
    static std::shared_ptr<ObjFunction> basic(std::shared_ptr<FunctionStmt> declaration, std::shared_ptr<Environment> closure, std::string owner_class_name = "") {
        return std::make_shared<ObjFunction>(declaration, closure, false, std::move(owner_class_name));
    }

    static std::shared_ptr<ObjFunction> init(std::shared_ptr<FunctionStmt> declaration, std::shared_ptr<Environment> closure, std::string owner_class_name = "") {
        return std::make_shared<ObjFunction>(declaration, closure, true, std::move(owner_class_name));
    }

    std::shared_ptr<ObjFunction> bind(ObjInstance *inst);

    ObjFunction(std::shared_ptr<FunctionStmt> declaration, std::shared_ptr<Environment> closure, const bool is_init, std::string owner_class_name = "")
        : Callable(ObjType::Function), declaration(std::move(declaration)), closure(std::move(closure)), is_init(is_init), owner_class_name(std::move(owner_class_name)) {}

    int arity() override;
    Value call(Vm *vm, const std::vector<Value> &args) override;
    bool operator==(const Obj &other) override;
    void print(std::ostream &os) override;
    Value index(const Value &key) override;

    std::shared_ptr<FunctionStmt> declaration;
    std::shared_ptr<Environment> closure;
    bool is_init;
    std::string owner_class_name;
};

using NativeFn = Value(*)(const std::vector<Value> &);

class ObjNative : public Callable {
public:
    ObjNative(int arity, NativeFn function);

    int arity() override;
    Value call(Vm *vm, const std::vector<Value> &args) override;
    bool operator==(const Obj &other) override;
    void print(std::ostream &os) override;
    Value index(const Value &key) override;

    int arity_value;
    NativeFn function;
};

class ObjClass : public Callable {
public:
    ObjClass(
        std::string name,
        std::map<std::string, std::shared_ptr<ObjFunction>> methods,
        std::set<std::string> private_fields = {},
        std::shared_ptr<ObjFunction> field_initializer = nullptr
    );

    std::shared_ptr<ObjFunction> find_method(const std::string &method_name) const;
    int arity() override;
    Value call(Vm *vm, const std::vector<Value> &args) override;
    bool operator==(const Obj &other) override;
    void print(std::ostream &os) override;
    Value index(const Value &key) override;

    std::string name;
    std::map<std::string, std::shared_ptr<ObjFunction>> methods;
    std::set<std::string> private_fields;
    std::shared_ptr<ObjFunction> field_initializer;
};

class ObjInstance : public Obj {
public:
    explicit ObjInstance(std::shared_ptr<ObjClass> klass);

    bool operator==(const Obj &other) override;
    void print(std::ostream &os) override;
    Value index(const Value &key) override;

    Value get(const std::string &field_name, const std::string &access_class_name = "");
    void set(const std::string &field_name, Value value, const std::string &access_class_name = "");

    std::shared_ptr<ObjClass> klass;
    std::map<std::string, Value> fields;
};

class List: public Obj {
public:
    explicit List(std::vector<Value> elements);

    bool operator==(const Obj &other) override;
    void print(std::ostream &os) override;
    Value index(const Value &key) override;
    Value set_index(const Value &key, const Value &value) override;

    std::vector<Value> elements;
};

class ObjModule : public Obj {
public:
    ObjModule(std::string path, std::unordered_map<std::string, Value> exports);

    bool operator==(const Obj &other) override;
    void print(std::ostream &os) override;
    Value index(const Value &key) override;

    Value get(const std::string &name) const;
    bool has(const std::string &name) const;

    std::string path;
    std::unordered_map<std::string, Value> exports;
};

class ObjError : public Obj {
public:
    explicit ObjError(std::string message);

    bool operator==(const Obj &other) override;
    void print(std::ostream &os) override;
    Value index(const Value &key) override;

    std::string message;
};

class Variadic: public Obj {
public:
    explicit Variadic(std::vector<Value> elements);

    bool operator==(const Obj &other) override;
    void print(std::ostream &os) override;
    Value index(const Value &key) override;

    std::vector<Value> elements;
};

#endif //DSCRIPT_OBJS_H