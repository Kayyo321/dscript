#include "obj.h"

#include <ostream>
#include <sstream>

#include "throwables.h"
#include "value.h"

Obj::Obj(const ObjType type) : type(type) {}

ObjType Obj::get_type() const {
    return type;
}

void Obj::print(std::ostream &os) {
    os << "<object>";
}

Value Obj::index(const Value &key){
    std::stringstream ss{"object '"};
    this->print(ss);
    ss << "' is not indexable.";
    
    throw RuntimeError(ss.str());
}
