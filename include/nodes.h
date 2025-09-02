#pragma once

#include "identifiable.h"
#include "magnetic.h"
#include "radial.h"

#include <iostream>

class BasicNode : public Magnetic, public Identifiable{
    public:
        BasicNode() : Magnetic{0}, Identifiable{Identifiable::nullID}{};
        BasicNode(int id, double susceptibility) : Magnetic{susceptibility}, Identifiable{id}{};
    private:

};

class RadialNode : public Magnetic, public Radial, public Identifiable{
    public:
        RadialNode() : Magnetic{0}, Radial{1.0}, Identifiable{Identifiable::nullID}{};
        RadialNode(int id, double radius, double susceptibility) : Magnetic{susceptibility}, Radial{radius}, Identifiable{id}{};
    private:

};