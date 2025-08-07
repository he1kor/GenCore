#pragma once

#include <utility>
#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "2d.h"

class RasterLine {
private:
    std::vector<IntVector2> points;
    size_t length;

    // Bresenham's line algorithm to generate all points
    void generateLinePoints(IntVector2 p1, IntVector2 p2) {
        int x1 = p1.x, y1 = p1.y;
        int x2 = p2.x, y2 = p2.y;

        int dx = abs(x2 - x1);
        int dy = abs(y2 - y1);
        int sx = (x1 < x2) ? 1 : -1;
        int sy = (y1 < y2) ? 1 : -1;
        int err = dx - dy;

        while (true) {
            points.emplace_back(x1, y1);
            if (x1 == x2 && y1 == y2) break;
            
            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x1 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y1 += sy;
            }
        }
        
        length = points.size();
    }

public:
    RasterLine(IntVector2 p1, IntVector2 p2) {
        generateLinePoints(p1, p2);
    }

    size_t size() const { return length; }

    IntVector2 operator[](size_t i) const {
        if (i >= length) {
            throw std::out_of_range("Index out of range in RasterLine");
        }
        return points[i];
    }
};