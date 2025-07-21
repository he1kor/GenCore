#include "2d.h"


DoubleVector2 DoubleVector2::operator*(double scalar) const{
    return {x * scalar, y * scalar};
}
