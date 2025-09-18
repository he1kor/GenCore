#include "border.h"

#include <stdexcept>
#include <format>

using namespace tiles;

Border::Edge::Edge(IntVector2 aTile, IntVector2 bTile){
    if (aTile.x == bTile.x){
        if (abs(aTile.y - bTile.y) != 1){
            throw std::invalid_argument("Given tiles are not 4-connected neighbours");
        }
        orientation = Orientation::vertical;
        topLeftPos = {aTile.x, std::min(aTile.y, bTile.y)};
    }
    else if (aTile.y == bTile.y){
        if (abs(aTile.x - bTile.x) != 1){
            throw std::invalid_argument("Given tiles are not 4-connected neighbours");
        }
        orientation = Orientation::horizontal;
        topLeftPos = {std::min(aTile.x, bTile.x), aTile.y};
    }
    else {
        throw std::invalid_argument("Given tiles are not 4-connected neighbours");
    }
}

Border::Edge::Edge(const Segment& segment){
    orientation =
        segment.direction == Direction::right || segment.direction == Direction::left ? Orientation::vertical : Orientation::horizontal;

    this->topLeftPos =
        segment.direction == Direction::right || segment.direction == Direction::up ? segment.getLeftPos() : segment.getRightPos();
}

IntVector2 Border::Edge::getTopLeftTile() const{
    return topLeftPos;
}

IntVector2 Border::Edge::getbottomRightTile() const{
    IntVector2 result = topLeftPos;
    switch (orientation){
        case Orientation::horizontal:
            result.x++;
            break;
        case Orientation::vertical:
            result.y++;
            break;
    }
    return result;
}

Border::Segment::Segment(IntVector2 aTile, IntVector2 bTile){
    if (aTile.x == bTile.x){
        int dif = bTile.y - aTile.y;
        if (dif == 1){
            direction == Direction::down;
        } else if (dif == -1){
            direction == Direction::up;
        } else{
            throw std::invalid_argument("Given tiles are not 4-connected neighbours");
        }
    }
    else if (aTile.y == bTile.y){
        int dif = bTile.x - aTile.x;
        if (dif == 1){
            direction == Direction::right;
        } else if (dif == -1){
            direction == Direction::left;
        } else{
            throw std::invalid_argument("Given tiles are not 4-connected neighbours");
        }
    }
    else {
        throw std::invalid_argument("Given tiles are not 4-connected neighbours");
    }

    startPos = aTile;
}

tiles::Border::Segment::Segment(const Edge &edge){
    IntVector2 pos = edge.getTopLeftTile();
    if (edge.orientation == Orientation::horizontal){
        pos.x++;
        direction = Direction::down;
    } else{
        pos.y++;
        direction = Direction::right;
    }
    startPos = pos;
}

void Border::Segment::turnLeft(){
    ::turnLeft(direction);
}

void Border::Segment::turnRight(){
    ::turnRight(direction);
}

void tiles::Border::Segment::turnBackward(){
    ::turnBackward(direction);
}

IntVector2 tiles::Border::Segment::getLeftPos() const{
    IntVector2 result = startPos;
    switch (direction){
        case Direction::up:
            result.x--;
            result.y--;
            break;
        case Direction::left:
            result.x--;
            break;
        case Direction::right:
            result.y--;
            break;
        case Direction::down:
            break;
    }
    return result;
}

IntVector2 tiles::Border::Segment::getRightPos() const{
    IntVector2 result = startPos;
    switch (direction){
        case Direction::right:
            break;
        case Direction::down:
            result.x--;
            break;
        case Direction::up:
            result.y--;
            break;
        case Direction::left:
            result.x--;
            result.y--;
            break;
    }
    return result;
}

void Border::Segment::moveForward(){
    setPos(getNextPos());
}

bool Border::Segment::fits(Edge &edge) const{
    if (edge.orientation != toOrientation(direction))
        return false;
    const IntVector2& topLeftPos =
        direction == Direction::right || direction == Direction::down ? startPos : getNextPos();
    return topLeftPos == edge.topLeftPos;
}

std::array<Border::Segment, 6> Border::Segment::getNeighbours() const{
    Border::Segment segment = *this;
    segment.moveForward();
    std::array<Border::Segment, 6> result ={
        *this,
        *this,
        *this,
        segment,
        segment,
        segment
    };
    result[0].turnLeft();
    result[1].turnBackward();
    result[2].turnRight();
    
    result[3].turnLeft();
    result[5].turnRight();
    return result;
}

std::array<Border::Segment, 3> Border::Segment::getNext() const{
    Border::Segment segment = *this;
    segment.moveForward();
    std::array<Border::Segment, 3> result;
    result.fill(segment);
    result[0].turnLeft();
    result[2].turnRight();
    return result;
}

std::array<Border::Segment, 3> Border::Segment::getPrev() const{
    std::array<Border::Segment, 3> result;
    result.fill(*this);
    result[0].turnLeft();
    result[2].turnRight();
    return result;
}

void Border::Segment::setPos(IntVector2 pos){
    this->startPos = pos;
}

void Border::Segment::setDirection(Direction direction){
    this->direction = direction;
}

size_t tiles::Border::manhattanFromTo(size_t i1, size_t i2) const{
    int xDif = segments.at(i2).getNextPos().x - segments.at(i2).getStartPos().x;
    int yDif = segments.at(i2).getNextPos().y - segments.at(i2).getStartPos().y;
    return std::abs(xDif) + std::abs(yDif);
}

IntVector2 Border::Segment::getNextPos() const{
    IntVector2 move;
    switch (direction){
        case Direction::up:
            move = {0, -1};
            break;
        case Direction::left:
            move = {-1, 0};
            break;
        case Direction::right:
            move = {1, 0};
            break;
        case Direction::down:
            move = {0, 1};
            break;
    }
    return startPos + move;
}

void tiles::Border::initPassData(size_t minPassWidth, size_t maxPassWidth){
    this->minPassWidth = minPassWidth;
    this->maxPassWidth = maxPassWidth;

    if (maxPassWidth < minPassWidth){
        throw std::invalid_argument(std::format("maxPassWidth ({}) is greater than minPassWidth ({})", maxPassWidth, minPassWidth));
    }
    size_t numberPasses = maxPassWidth - minPassWidth + 1;

    std::vector<size_t> currentFurthestIndex(numberPasses, 0);
    for (size_t i = 0; i < segments.size(); i++){
        nextPassIndex[i] = std::vector<std::optional<size_t>>(numberPasses, std::nullopt);
        if (i == 0 || i == segments.size() - 1){
            continue;
        }
        for (size_t j = i+1; j < segments.size() - 1; j++){
            size_t distance = manhattanFromTo(i, j);
            size_t widthIndex = distance - minPassWidth;
            if (distance <= maxPassWidth && distance >= minPassWidth && j > currentFurthestIndex[widthIndex]){
                nextPassIndex[i][widthIndex] = j;
                currentFurthestIndex[widthIndex] = j;
            }
        }
    }
}

const Border::Segment &Border::at(size_t i) const{
    return segments.at(i);
}

std::vector<Border::PossiblePass> tiles::Border::possiblePasses(size_t index) const{
    std::vector<Border::PossiblePass> result;
    for (size_t i = 0; i <= maxPassWidth - minPassWidth; i++){
        if (const std::optional<size_t>& j = nextPassIndex.at(index)[i]){
            result.emplace_back(index, *j, i+minPassWidth);
        }
    }
    return result;
}

IntVector2 tiles::Border::getLeft(size_t i){
    return segments[i].getLeftPos();
}

IntVector2 tiles::Border::getRight(size_t i){
    return segments[i].getRightPos();
}

std::size_t tiles::Border::Edge::Hash::operator()(const Edge& segment) const{
    std::hash<int> int_hasher;
    std::size_t pos_hash = int_hasher(segment.topLeftPos.x);
    pos_hash ^= int_hasher(segment.topLeftPos.y) + 0x9e3779b9 + (pos_hash << 6) + (pos_hash >> 2);
    
    std::size_t dir_hash = static_cast<std::size_t>(segment.orientation);
    
    return pos_hash ^ (dir_hash + 0x9e3779b9 + (pos_hash << 6) + (pos_hash >> 2));
}


std::size_t tiles::Border::Segment::Hash::operator()(const Segment& segment) const{
    std::hash<int> int_hasher;
    std::size_t pos_hash = int_hasher(segment.startPos.x);
    pos_hash ^= int_hasher(segment.startPos.y) + 0x9e3779b9 + (pos_hash << 6) + (pos_hash >> 2);
    
    std::size_t dir_hash = static_cast<std::size_t>(segment.direction);
    
    return pos_hash ^ (dir_hash + 0x9e3779b9 + (pos_hash << 6) + (pos_hash >> 2));
}
