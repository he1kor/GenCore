#pragma once

#include "identifiable.h"
#include "magnetic.h"

#include <iostream>

class MagneticNode : public Magnetic, public Identifiable{
    public:
        MagneticNode() : Magnetic{0}, Identifiable{Identifiable::nullID}{};
        MagneticNode(int id, double suspectibility) : Magnetic{suspectibility}, Identifiable{id}{};
    private:

};