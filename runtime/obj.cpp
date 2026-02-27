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
    
    throw IndexError(ss.str());
}

Value Obj::set_index(const Value &key, const Value &value) {
    (void) key;
    (void) value;

    std::stringstream ss{"object '"};
    this->print(ss);
    ss << "' does not support index assignment.";

    throw IndexError(ss.str());
}
