#pragma once

#include <utility>
#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "2d.h"

class RasterLine {
    DoubleVector2 start;
    DoubleVector2 end;
    bool steep;
    int dx, dy, ystep;
    int length;
    
public:
    RasterLine(DoubleVector2 p1, DoubleVector2 p2) : start(p1), end(p2) {
        int x1 = p1.x, y1 = p1.y;
        int x2 = p2.x, y2 = p2.y;
        
        steep = std::abs(y2 - y1) > std::abs(x2 - x1);
        if (steep) {
            std::swap(x1, y1);
            std::swap(x2, y2);
        }
        
        if (x1 > x2) {
            std::swap(x1, x2);
            std::swap(y1, y2);
        }
        
        dx = x2 - x1;
        dy = std::abs(y2 - y1);
        ystep = (y1 < y2) ? 1 : -1;
        length = dx + 1;
    }
    
    int size() const { return length; }
    
    DoubleVector2 operator[](int i) const {
        if (i < 0 || i >= length) 
            throw std::out_of_range("Line index out of range");
            
        int x1 = steep ? start.y : start.x;
        int y1 = steep ? start.x : start.y;
        
        int x = x1 + i;
        int error = (i * dy) % dx;
        int y = y1 + (i * dy) / dx * ystep;
        
        if (2*error > dx) {
            y += ystep;
        }
        
        return steep ? DoubleVector2{y, x} : DoubleVector2{x, y};
    }
};