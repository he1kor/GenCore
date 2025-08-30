#include "bloat_strategy.h"
#include "radial.h"
#include <stdexcept>

void bloatStrategies::DiagonalRandomBloat::bloat(const ZoneTile &activeTile){
    int x = activeTile.x;
    int y = activeTile.y;

    auto locked = this->zoneTilePusher.lock();
    if (!locked.has_value())
        throw std::logic_error("ZoneTilePusher is nullptr");
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

void bloatStrategies::StraightBloat::bloat(const ZoneTile &activeTile){
    int x = activeTile.x;
    int y = activeTile.y;

    auto locked = this->zoneTilePusher.lock();
    if (!locked.has_value())
        throw std::logic_error("ZoneTilePusher is nullptr");
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

void bloatStrategies::RandomBloat::bloat(const ZoneTile &activeTile){
    int x = activeTile.x;
    int y = activeTile.y;

    auto locked = this->zoneTilePusher.lock();
    if (!locked.has_value())
        throw std::logic_error("ZoneTilePusher is nullptr");
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

void bloatStrategies::ChebyshevBloat::bloat(const ZoneTile &activeTile){
    int x = activeTile.x;
    int y = activeTile.y;

    auto locked = this->zoneTilePusher.lock();
    if (!locked.has_value())
        throw std::logic_error("ZoneTilePusher is nullptr");
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
    if (ref.isValidPoint(tempPoint))
        ref.push(ZoneTile(tempPoint, activeTile.zoneID));

    tempPoint = {x + 1, y - 1};
    if (ref.isValidPoint(tempPoint))
        ref.push(ZoneTile(tempPoint, activeTile.zoneID));

    tempPoint = {x + 1, y + 1};
    if (ref.isValidPoint(tempPoint))
        ref.push(ZoneTile(tempPoint, activeTile.zoneID));

    tempPoint = {x - 1, y + 1};
    if (ref.isValidPoint(tempPoint))
        ref.push(ZoneTile(tempPoint, activeTile.zoneID));
}

void bloatStrategies::AdjacentCornerFill::bloat(const ZoneTile &activeTile){
    int x = activeTile.x;
    int y = activeTile.y;

    auto locked = this->zoneTilePusher.lock();
    if (!locked.has_value())
        throw std::logic_error("ZoneTilePusher is nullptr");
    auto& ref = locked->get();

    if (!ref.isEmpty({x, y})){
        return;
    }

    auto left = ref.tryGetID({x-1, y}).value_or(Identifiable::nullID);
    auto upper = ref.tryGetID({x, y-1}).value_or(Identifiable::nullID);
    auto right = ref.tryGetID({x+1, y}).value_or(Identifiable::nullID);
    auto bottom = ref.tryGetID({x, y+1}).value_or(Identifiable::nullID);

    bool upperLeft = upper != Identifiable::nullID && upper == left;
    bool upperRight = upper != Identifiable::nullID && upper == right;
    bool bottomRight = bottom != Identifiable::nullID && bottom == right;
    bool bottomLeft = bottom != Identifiable::nullID && bottom == left;

    //if No adjacent corners
    if (!(upperLeft || upperRight || bottomRight || bottomLeft))
        return;

    //set tile from adjacent corner
    if (upperLeft || bottomLeft)
        ref.setZoneTile({x, y, left});
    if (upperRight || bottomRight)
        ref.setZoneTile({x, y, right});

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
