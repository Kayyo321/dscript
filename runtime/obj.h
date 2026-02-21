//
// Created by sullivanb on 2/20/26.
//

#ifndef DSCRIPT_OBJ_H
#define DSCRIPT_OBJ_H

#include <iosfwd>

enum class ObjType {
    Class,
    Function,
    Instance,
    Native,
    String,
};

class Obj {
public:
    explicit Obj(ObjType type);
    virtual ~Obj() = default;

    ObjType get_type() const;

    virtual bool operator==(const Obj &other) = 0;
    virtual void print(std::ostream &os);

private:
    ObjType type;
};

#endif //DSCRIPT_OBJ_H