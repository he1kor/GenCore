#include "2d.h"

using namespace tiles;
void tiles::turnRight(Direction &direction){
    switch (direction){
    case Direction::down:
        direction = Direction::left;
        break;
    case Direction::left:
        direction = Direction::up;
        break;
    case Direction::up:
        direction = Direction::right;
        break;
    case Direction::right:
        direction = Direction::down;
        break;
    }
}

void tiles::turnLeft(Direction &direction){
    switch (direction){
    case Direction::down:
        direction = Direction::right;
        break;
    case Direction::left:
        direction = Direction::down;
        break;
    case Direction::up:
        direction = Direction::left;
        break;
    case Direction::right:
        direction = Direction::up;
        break;
    }
}
    
void tiles::turnBackward(Direction &direction){
    switch (direction){
    case Direction::down:
        direction = Direction::up;
        break;
    case Direction::left:
        direction = Direction::right;
        break;
    case Direction::up:
        direction = Direction::down;
        break;
    case Direction::right:
        direction = Direction::left;
        break;
    }
}

Orientation tiles::toOrientation(Direction direction){
    return direction == Direction::left || direction == Direction::right ?
        Orientation::horizontal :
        Orientation::vertical;
}

DoubleVector2 DoubleVector2::operator*(double scalar) const{
    return {x * scalar, y * scalar};
}

DoubleVector2 DoubleVector2::operator+(const DoubleVector2 &other) const{
    return {x + other.x, y + other.y};
}

DoubleVector2 DoubleVector2::operator-(const DoubleVector2 &other) const{
    return {x - other.x, y - other.y};
}



IntVector2 IntVector2::operator*(int scalar) const{
    return {x * scalar, y * scalar};
}

IntVector2 IntVector2::operator+(const IntVector2 &other) const{
    return {x + other.x, y + other.y};
}

IntVector2 IntVector2::operator-(const IntVector2 &other) const{
    return {x - other.x, y - other.y};
}

bool IntVector2::operator==(const IntVector2 &other) const{
    return x == other.x && y == other.y;
}

std::ostream& operator<<(std::ostream &os, const IntVector2 &vec){
    os << "(" << vec.x << ", " << vec.y << ")";
    return os;
}