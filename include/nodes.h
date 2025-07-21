#pragma once

#include "identifiable.h"
#include "magnetic.h"

#include <iostream>

class BasicNode : public Magnetic, public Identifiable{
    public:
        BasicNode() : Magnetic{0}, Identifiable{Identifiable::nullID}{};
        BasicNode(int id, double suspectibility) : Magnetic{suspectibility}, Identifiable{id}{};
    private:

};