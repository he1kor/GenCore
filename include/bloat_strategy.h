#pragma once

#include <functional>
#include <queue>
#include <memory>

#include "2d.h"
#include "edge_graph.h"
#include "identifiable.h"
#include <iostream>


#include "line.h"
#include "radial.h"
#include "random_generator.h"

#include "self_pointer.h"

struct ZoneTile : IntVector2{
    ZoneTile(IntVector2 point, Identifiable id) : IntVector2{point.x, point.y}, zoneID(id){};
    ZoneTile(int x, int y, Identifiable id) : IntVector2{x, y}, zoneID(id){};
    Identifiable zoneID;
};

class ZoneTilePusher : public SelfPointerOwner<ZoneTilePusher>{
public:
    virtual void push(const ZoneTile& zoneTile) = 0;

    virtual void setZoneTile(const ZoneTile& zoneTile) = 0;
    virtual bool isEmpty(IntVector2 point) const = 0;
    virtual bool isValidPoint(IntVector2 point) const = 0;
};

class BloatStrategy{
public:
    virtual ~BloatStrategy() = default;

    BloatStrategy() : zoneTilePusher{nullptr}{};
    BloatStrategy(SelfPointer<ZoneTilePusher> zoneTilePusher) : zoneTilePusher{zoneTilePusher}{};

    BloatStrategy(const BloatStrategy&) = delete;
    BloatStrategy& operator=(const BloatStrategy&) = delete;
    
    BloatStrategy(BloatStrategy&&) = default;
    BloatStrategy& operator=(BloatStrategy&&) = default;

    virtual void bloat(const ZoneTile& activeTile) = 0;

    void setZoneTilePusher(SelfPointer<ZoneTilePusher> zoneTilePusher){this->zoneTilePusher = zoneTilePusher;};
protected:
    SelfPointer<ZoneTilePusher> zoneTilePusher;
};


namespace bloatStrategies{
        
    class StraightBloatStrategy : public BloatStrategy{
    public:
        StraightBloatStrategy(SelfPointer<ZoneTilePusher> zoneTilePusher) : BloatStrategy(zoneTilePusher){};
        StraightBloatStrategy() : BloatStrategy(){};

        
    
        void bloat(const ZoneTile &activeTile) override;
    };


    class DiagonalRandomBloatStrategy : public BloatStrategy{
    public:
        const double diagonalChance;

        DiagonalRandomBloatStrategy() :
            BloatStrategy(),
            diagonalChance{0.4}{};
        DiagonalRandomBloatStrategy(double diagonalChance) :
            BloatStrategy(),
            diagonalChance{diagonalChance}{};

        DiagonalRandomBloatStrategy(SelfPointer<ZoneTilePusher> zoneTilePusher) :
            BloatStrategy(zoneTilePusher),
            diagonalChance{0.4}{};
        DiagonalRandomBloatStrategy(SelfPointer<ZoneTilePusher> zoneTilePusher, double diagonalChance) :
            BloatStrategy(zoneTilePusher),
            diagonalChance{diagonalChance}{};

        void bloat(const ZoneTile& activeTile) override;
    };

    class RandomBloatStrategy : public BloatStrategy{
    public:
        const double randomChance;

        RandomBloatStrategy(const RandomBloatStrategy&) = delete;
        RandomBloatStrategy& operator=(const RandomBloatStrategy&) = delete;
    
        RandomBloatStrategy(RandomBloatStrategy&&) = default;
        RandomBloatStrategy& operator=(RandomBloatStrategy&&) = default;

        RandomBloatStrategy(double diagonalChance) :
            BloatStrategy(),
            randomChance{randomChance}{};
        RandomBloatStrategy() :
            BloatStrategy(),
            randomChance{0.5}{};
        RandomBloatStrategy(SelfPointer<ZoneTilePusher> ZoneTilePusher) :
            BloatStrategy(zoneTilePusher),
            randomChance{0.5}{};
        RandomBloatStrategy(SelfPointer<ZoneTilePusher> zoneTilePusher, double diagonalChance) :
            BloatStrategy(zoneTilePusher),
            randomChance{randomChance}{};

        void bloat(const ZoneTile& activeTile) override;
    };
}