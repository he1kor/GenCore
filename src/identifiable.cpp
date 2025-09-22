#include "identifiable.h"

Identifiable::Identifiable(int id) : id{id}{}

int Identifiable::getID() const{
    return id;
}

bool Identifiable::operator!=(const Identifiable &other) const{
    return !(*this == other);
}

bool Identifiable::operator==(const Identifiable &other) const{
    return id == other.id;
}

bool Identifiable::operator>(const Identifiable &other) const{
    return id > other.id;
}

bool Identifiable::operator<(const Identifiable &other) const{
    return id < other.id;
}

bool Identifiable::operator>=(const Identifiable &other) const{
    return id >= other.id;
}

bool Identifiable::operator<=(const Identifiable &other) const{
    return id <= other.id;
}

std::ostream &operator<<(std::ostream &os, const Identifiable &id){
    if (id.getID() == Identifiable::nullID)
        os << "[Null_ID]";
    else
        os << id.getID() << "_ID";
    return os;
}
