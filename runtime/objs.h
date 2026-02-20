//
// Created by sullivanb on 2/20/26.
//

#ifndef DSCRIPT_OBJS_H
#define DSCRIPT_OBJS_H

#include <map>

#include "obj.h"

#include "value.h"

#include <string>
#include <map>
#include <vector>

class ObjString : public Obj {
public:
    explicit ObjString(std::string chars);

    bool operator==(const Obj &other) override;
    void print(std::ostream &os) override;

    std::string chars;
};

class ObjFunction : public Obj {
public:
    //TODO chunk = AST Function
    ObjFunction();

    bool operator==(const Obj &other) override;
    void print(std::ostream &os) override;

    int arity;
    int upvalueCount;
    //Chunk chunk;
    ObjString *name;
};

using NativeFn = Value(*)(std::vector<std::shared_ptr<ObjFunction>>);

class ObjNative : public Obj {
public:
    explicit ObjNative(NativeFn function);

    bool operator==(const Obj &other) override;
    void print(std::ostream &os) override;

    NativeFn function;
};

class ObjClass : public Obj {
public:
    ObjClass(std::string name, std::map<std::string, std::shared_ptr<ObjFunction>> methods);

    bool operator==(const Obj &other) override;
    void print(std::ostream &os) override;

    std::string name;
    std::map<std::string, std::shared_ptr<ObjFunction>> methods;
};

class ObjInstance : public Obj {
public:
    explicit ObjInstance(ObjClass *klass, std::map<std::string, Value> fields);

    bool operator==(const Obj &other) override;
    void print(std::ostream &os) override;

    ObjClass *klass;
    std::map<std::string, Value> fields;
};

#endif //DSCRIPT_OBJS_H