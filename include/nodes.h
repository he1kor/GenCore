#pragma once

#include "identifiable.h"
#include "magnetic.h"
#include "radial.h"

#include <iostream>
#include "resource_generator.h"

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

template <typename Resource>
struct ResourceData{
    std::vector<NoiseOctaveParam> octaves;
    std::vector<ResourceMapping<Resource>> resources;
};

template <typename Resource>
class ResourceRadialNode : public RadialNode, public ResourceData<Resource>{
    public: 
        ResourceRadialNode() : RadialNode(), ResourceData<Resource>{.octaves = {}, .resources = {}}{};
        ResourceRadialNode(
            int id,
            double radius,
            double susceptibility,
            const std::vector<NoiseOctaveParam>& octaves,
            const std::vector<ResourceMapping<Resource>>& resources
        ) : RadialNode(id, radius, susceptibility), ResourceData<Resource>{octaves, resources}{};
};