#pragma once

#include "identifiable.h"
#include <unordered_map>
#include <memory>
#include <type_traits>
#include <border.h>

class Blendable{
public:
    Blendable(int distance) : blendDistance(distance){}
    int blendDistance = 0;
};

class ResourceGuarantor {
public:
    ResourceGuarantor(int areaGuaranteed, double bonusValue) : areaGuaranteed(areaGuaranteed), bonusValue(bonusValue) {}
    int areaGuaranteed = 0;
    double bonusValue = 0.0;
};

class Intakable{
public:
    Intakable(int intakeDistance) : intakeDistance(intakeDistance){}
    int intakeDistance = 0;
};

class BasicAsymConnection : public ResourceGuarantor, public Intakable {
public:
    BasicAsymConnection() : Intakable(0), ResourceGuarantor(0, 0.0){};
    BasicAsymConnection(int intakeDistance, int areaGuaranteed, double bonusValue) : Intakable(intakeDistance), ResourceGuarantor(areaGuaranteed, bonusValue){};
};

struct BasicSymConnection : public Blendable, public tiles::PassParams{
    BasicSymConnection() : Blendable(0), tiles::PassParams(){};
    BasicSymConnection(int blendableDistance, const tiles::PassParams& params) : Blendable(blendableDistance), tiles::PassParams(params){};
};