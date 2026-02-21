#include "obj.h"

#include <ostream>

Obj::Obj(const ObjType type) : type(type) {}

ObjType Obj::get_type() const {
    return type;
}

void Obj::print(std::ostream &os) {
    os << "<object>";
}
