#include "identifiable.h"

Identifiable::Identifiable(int id) : id{id}{}

int Identifiable::getID() const
{
    return id;
}