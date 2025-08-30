#pragma once

#include <functional>
#include <queue>
#include <memory>
#include <optional>

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
    virtual std::optional<Identifiable> tryGetID(IntVector2 point) const = 0;
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
        
    class StraightBloat : public BloatStrategy{
    public:
        StraightBloat(SelfPointer<ZoneTilePusher> zoneTilePusher) : BloatStrategy(zoneTilePusher){};
        StraightBloat() : BloatStrategy(){};

        
    
        void bloat(const ZoneTile &activeTile) override;
    };

    class ChebyshevBloat : public BloatStrategy{
    public:
        ChebyshevBloat(SelfPointer<ZoneTilePusher> zoneTilePusher) : BloatStrategy(zoneTilePusher){};
        ChebyshevBloat() : BloatStrategy(){};

        void bloat(const ZoneTile &activeTile) override;
    };

    class DiagonalRandomBloat : public BloatStrategy{
    public:
        const double diagonalChance;

        DiagonalRandomBloat() :
            BloatStrategy(),
            diagonalChance{0.4}{};
        DiagonalRandomBloat(double diagonalChance) :
            BloatStrategy(),
            diagonalChance{diagonalChance}{};

        DiagonalRandomBloat(SelfPointer<ZoneTilePusher> zoneTilePusher) :
            BloatStrategy(zoneTilePusher),
            diagonalChance{0.4}{};
        DiagonalRandomBloat(SelfPointer<ZoneTilePusher> zoneTilePusher, double diagonalChance) :
            BloatStrategy(zoneTilePusher),
            diagonalChance{diagonalChance}{};

        void bloat(const ZoneTile& activeTile) override;
    };

    class RandomBloat : public BloatStrategy{
    public:
        const double randomChance;

        RandomBloat(const RandomBloat&) = delete;
        RandomBloat& operator=(const RandomBloat&) = delete;
    
        RandomBloat(RandomBloat&&) = default;
        RandomBloat& operator=(RandomBloat&&) = default;

        RandomBloat(double diagonalChance) :
            BloatStrategy(),
            randomChance{randomChance}{};
        RandomBloat() :
            BloatStrategy(),
            randomChance{0.5}{};
        RandomBloat(SelfPointer<ZoneTilePusher> ZoneTilePusher) :
            BloatStrategy(zoneTilePusher),
            randomChance{0.5}{};
        RandomBloat(SelfPointer<ZoneTilePusher> zoneTilePusher, double diagonalChance) :
            BloatStrategy(zoneTilePusher),
            randomChance{randomChance}{};

        void bloat(const ZoneTile& activeTile) override;
    };

    class AdjacentCornerFill : public BloatStrategy{
    public:
        AdjacentCornerFill(SelfPointer<ZoneTilePusher> zoneTilePusher) : BloatStrategy(zoneTilePusher){};
        AdjacentCornerFill() : BloatStrategy(){};

        void bloat(const ZoneTile &activeTile) override;
    };
}