#pragma once

#include "identifiable.h"
#include <unordered_map>
#include <memory>
#include <type_traits>

class Blendable{
public:
    Blendable(int distance) : blendDistance(distance){}
    int blendDistance = 0;
};

class ResourceGuarantor {
public:
    ResourceGuarantor(int areaGuaranteed, double bonuxValue) : areaGuaranteed(areaGuaranteed), bonusValue(bonusValue) {}
    int areaGuaranteed = 0;
    double bonusValue = 0.0;
};

class Intakable{
public:
    Intakable(int intakeDistance) : intakeDistance(intakeDistance){}
    int intakeDistance = 0;
};

class BasicConnection : public Blendable, public ResourceGuarantor, public Intakable {
public:
    BasicConnection() : Blendable(0), Intakable(0), ResourceGuarantor(0, 0.0){};
    BasicConnection(int blendableDistance, int intakeDistance, int areaGuaranteed, double bonusValue) : Blendable(blendableDistance), Intakable(intakeDistance), ResourceGuarantor(areaGuaranteed, bonusValue){};
}; 