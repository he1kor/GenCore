#pragma once

#include <functional>
#include <stdexcept>

#include "grid.h"
#include "matrix.h"

#include <iostream>

namespace morphology{
    void placeKernel(Matrix<bool>& grid, typename Matrix<bool>::Iterator it, const Matrix<bool>& kernel);
    bool canErodeKernel(Matrix<bool>& grid, typename Matrix<bool>::Iterator it, const Matrix<bool>& kernel);
    void dilate(Matrix<bool>& boolMap, const Matrix<bool>& kernel);
    void erode(Matrix<bool>& boolMap, const Matrix<bool>& kernel);
    void close(Matrix<bool>& boolMap, const Matrix<bool>& kernel);
    void open(Matrix<bool>& boolMap, const Matrix<bool>& kernel);

    template <typename T>
    void dilate(Grid<T>& grid, Identifiable tileToApply, const Matrix<bool>& kernel);

    template <typename T>
    void close(Grid<T>& grid, Identifiable tileToApply, const Matrix<bool>& kernel);

    template <typename T>
    void open(Grid<T>& grid, Identifiable tileToApply, const Matrix<bool>& kernel);
    
    template <typename T>
    void closeForAll(Grid<T>& grid, std::vector<Identifiable> tileToApply, const Matrix<bool>& kernel);

    template <typename T>
    void openForAll(Grid<T> &grid, std::vector<Identifiable> tilesToApply, const Matrix<bool> &kernel);

    template <typename T>
    void apply(Grid<T>& grid, const Matrix<bool>& boolMap, Identifiable tile);

    void placeKernel(Matrix<bool>& grid, typename Matrix<bool>::Iterator it, const Matrix<bool>& kernel){

        int halfWidth = kernel.getWidth() / 2;
        int halfHeight = kernel.getHeight() / 2;

        int kernelStartX = std::max(0, halfWidth - it.getX());
        int kernelStartY = std::max(0, halfHeight - it.getY());
        int kernelEndX = std::min(kernel.getWidth(), halfWidth - it.getX() + grid.getWidth());
        int kernelEndY = std::min(kernel.getHeight(), halfHeight - it.getY() + grid.getHeight());

        int absStartX = it.getX() - halfWidth;
        int absStartY = it.getY() - halfHeight;

        for (size_t ry = kernelStartY; ry < kernelEndY; ry++){
            for (size_t rx = kernelStartX; rx < kernelEndX; rx++){
                if (!kernel.get(rx, ry)){
                    continue;
                }
                grid.set(
                    absStartX + rx,
                    absStartY + ry,
                    true
                );
            }
        }
    }

    bool canErodeKernel(Matrix<bool> &grid, typename Matrix<bool>::Iterator it, const Matrix<bool> &kernel){
        if (!*it)
            return false;
        int halfWidth = kernel.getWidth() / 2;
        int halfHeight = kernel.getHeight() / 2;

        int kernelStartX = std::max(0, halfWidth - it.getX());
        int kernelStartY = std::max(0, halfHeight - it.getY());
        int kernelEndX = std::min(kernel.getWidth(), halfWidth - it.getX() + grid.getWidth());
        int kernelEndY = std::min(kernel.getHeight(), halfHeight - it.getY() + grid.getHeight());

        int absStartX = it.getX() - halfWidth;
        int absStartY = it.getY() - halfHeight;

        for (size_t ry = kernelStartY; ry < kernelEndY; ry++){
            for (size_t rx = kernelStartX; rx < kernelEndX; rx++){
                if (kernel.get(rx, ry) && !grid.get(absStartX + rx, absStartY + ry)){
                    return true;
                }
            }
        }
        return false;
    }

    void dilate(Matrix<bool>& boolMap, const Matrix<bool>& kernel){
        if (kernel.getWidth() % 2 != 1 && kernel.getHeight() % 2 != 1){
            throw std::invalid_argument("The pattern size must be odd\n");
        }
        auto temp = boolMap;
        for (auto it = boolMap.begin(); it != boolMap.end(); ++it){
            if (temp.get(it.getX(), it.getY())){
                placeKernel(boolMap, it, kernel);
            }
        }
    }

    void erode(Matrix<bool>& boolMap, const Matrix<bool>& kernel){
        if (kernel.getWidth() % 2 != 1 && kernel.getHeight() % 2 != 1){
            throw std::invalid_argument("The pattern size must be odd\n");
        }
        auto temp = boolMap;
        for (auto it = boolMap.begin(); it != boolMap.end(); ++it){
            if (canErodeKernel(temp, it, kernel)){
                boolMap.set(it.getX(), it.getY(), false);
            }
        }
    }

    void close(Matrix<bool>& boolMap, const Matrix<bool>& kernel){
        dilate(boolMap, kernel);
        erode(boolMap, kernel);
    }

    void open(Matrix<bool>& boolMap, const Matrix<bool>& kernel){
        erode(boolMap, kernel);
        dilate(boolMap, kernel);
    }

    template <typename T>
    void dilate(Grid<T>& grid, Identifiable tileToApply, const Matrix<bool>& kernel){
        if (kernel.getWidth() % 2 != 1 && kernel.getHeight() % 2 != 1){
            throw std::invalid_argument("The pattern size must be odd\n");
        }
        Matrix<bool> boolMap(grid.getWidth(), grid.getHeight());

        for (auto it = grid.begin(); it != grid.end(); ++it){
            if (*it == tileToApply){
                boolMap.set(it.getX(), it.getY(), true);
            }
        }
        dilate(boolMap, kernel);
        apply(grid, boolMap, tileToApply);
    }

    template <typename T>
    void close(Grid<T>& grid, Identifiable tileToApply, const Matrix<bool>& kernel){
        if (kernel.getWidth() % 2 != 1 && kernel.getHeight() % 2 != 1){
            throw std::invalid_argument("The pattern size must be odd\n");
        }
        Matrix<bool> boolMap(grid.getWidth(), grid.getHeight());

        for (auto it = grid.begin(); it != grid.end(); ++it){
            if (*it == tileToApply){
                boolMap.set(it.getX(), it.getY(), true);
            }
        }
        close(boolMap, kernel);
        apply(grid, boolMap, tileToApply);
    }

    template <typename T>
    void open(Grid<T>& grid, Identifiable tileToApply, const Matrix<bool>& kernel){
        if (kernel.getWidth() % 2 != 1 && kernel.getHeight() % 2 != 1){
            throw std::invalid_argument("The pattern size must be odd\n");
        }
        Matrix<bool> boolMap(grid.getWidth(), grid.getHeight());

        for (auto it = grid.begin(); it != grid.end(); ++it){
            if (*it == tileToApply){
                boolMap.set(it.getX(), it.getY(), true);
            }
        }
        open(boolMap, kernel);
        apply(grid, boolMap, tileToApply);
    }

    template <typename T>
    void closeForAll(Grid<T> &grid, std::vector<Identifiable> tilesToApply, const Matrix<bool> &kernel){
        for (Identifiable tile : tilesToApply){
            close(grid, tile, kernel);
        }
    }

    template <typename T>
    void openForAll(Grid<T> &grid, std::vector<Identifiable> tilesToApply, const Matrix<bool> &kernel){
        for (Identifiable tile : tilesToApply){
            open(grid, tile, kernel);
        }
    }

    template <typename T>
    void apply(Grid<T>& grid, const Matrix<bool>& boolMap, Identifiable tile){
        for (auto it = grid.begin(); it != grid.end(); ++it){
            if (boolMap.get(it.getX(),it.getY())){
                *it = tile;
            } else if (*it == tile){
                *it = Identifiable::nullID;
            }
        }
    }

};