#include "bloat_strategy.h"
#include "radial.h"
#include <stdexcept>

void bloatStrategies::DiagonalRandomBloatStrategy::bloat(const ZoneTile &activeTile){
    int x = activeTile.x;
    int y = activeTile.y;

    auto locked = this->zoneTilePusher.lock();
    if (!locked.has_value())
        throw std::logic_error("ZoneTilePushed is nullptr");
    auto& ref = locked->get();

    if (!ref.isEmpty({x, y})){
        return;
    }


    ref.setZoneTile({x, y, activeTile.zoneID});

    IntVector2 tempPoint = {x - 1, y};
    if (ref.isValidPoint(tempPoint))
        ref.push(ZoneTile(tempPoint, activeTile.zoneID));

    tempPoint = {x + 1, y};
    if (ref.isValidPoint(tempPoint))
        ref.push(ZoneTile(tempPoint, activeTile.zoneID));

    tempPoint = {x, y - 1};
    if (ref.isValidPoint(tempPoint))
        ref.push(ZoneTile(tempPoint, activeTile.zoneID));

    tempPoint = {x, y + 1};
    if (ref.isValidPoint(tempPoint))
        ref.push(ZoneTile(tempPoint, activeTile.zoneID));


    tempPoint = {x - 1, y - 1};
    if (ref.isValidPoint(tempPoint) && RandomGenerator::instance().chanceOccurred(diagonalChance))
        ref.push(ZoneTile(tempPoint, activeTile.zoneID));

    tempPoint = {x - 1, y + 1};
    if (ref.isValidPoint(tempPoint) && RandomGenerator::instance().chanceOccurred(diagonalChance))
        ref.push(ZoneTile(tempPoint, activeTile.zoneID));

    tempPoint = {x + 1, y + 1};
    if (ref.isValidPoint(tempPoint) && RandomGenerator::instance().chanceOccurred(diagonalChance))
        ref.push(ZoneTile(tempPoint, activeTile.zoneID));

    tempPoint = {x + 1, y - 1};
    if (ref.isValidPoint(tempPoint) && RandomGenerator::instance().chanceOccurred(diagonalChance))
        ref.push(ZoneTile(tempPoint, activeTile.zoneID));
}

void bloatStrategies::StraightBloatStrategy::bloat(const ZoneTile &activeTile){
    int x = activeTile.x;
    int y = activeTile.y;

    auto locked = this->zoneTilePusher.lock();
    if (!locked.has_value())
        throw std::logic_error("ZoneTilePushed is nullptr");
    auto& ref = locked->get();

    if (!ref.isEmpty({x, y})){
        return;
    }

    ref.setZoneTile({x, y, activeTile.zoneID});

    IntVector2 tempPoint = {x - 1, y};
    if (ref.isValidPoint(tempPoint))
        ref.push(ZoneTile(tempPoint, activeTile.zoneID));

    tempPoint = {x + 1, y};
    if (ref.isValidPoint(tempPoint))
        ref.push(ZoneTile(tempPoint, activeTile.zoneID));

    tempPoint = {x, y - 1};
    if (ref.isValidPoint(tempPoint))
        ref.push(ZoneTile(tempPoint, activeTile.zoneID));

    tempPoint = {x, y + 1};
    if (ref.isValidPoint(tempPoint))
        ref.push(ZoneTile(tempPoint, activeTile.zoneID));
}

void bloatStrategies::RandomBloatStrategy::bloat(const ZoneTile &activeTile){
    int x = activeTile.x;
    int y = activeTile.y;

    auto locked = this->zoneTilePusher.lock();
    if (!locked.has_value())
        throw std::logic_error("ZoneTilePushed is nullptr");
    auto& ref = locked->get();


    if (!ref.isEmpty({x, y})){
        return;
    }
    if (!RandomGenerator::instance().chanceOccurred(randomChance)){
        ref.push(ZoneTile({x, y}, activeTile.zoneID));
        return;
    }
    ref.setZoneTile({x, y, activeTile.zoneID});

    IntVector2 tempPoint = {x - 1, y};
    if (ref.isValidPoint(tempPoint))
        ref.push(ZoneTile(tempPoint, activeTile.zoneID));

    tempPoint = {x + 1, y};
    if (ref.isValidPoint(tempPoint))
        ref.push(ZoneTile(tempPoint, activeTile.zoneID));

    tempPoint = {x, y - 1};
    if (ref.isValidPoint(tempPoint))
        ref.push(ZoneTile(tempPoint, activeTile.zoneID));

    tempPoint = {x, y + 1};
    if (ref.isValidPoint(tempPoint))
        ref.push(ZoneTile(tempPoint, activeTile.zoneID));
}