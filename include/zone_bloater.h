#pragma once

#include <memory>
#include <queue>

#include <iostream>

#include "grid.h"
#include "edge_graph.h"
#include "simulator.h"
#include "2d.h"
#include "identifiable.h"
#include "random_generator.h"

struct ZoneTile : IntVector2{
    ZoneTile(IntVector2 point, Identifiable id) : IntVector2{point.x, point.y}, zoneID(id){};
    ZoneTile(int x, int y, Identifiable id) : IntVector2{x, y}, zoneID(id){};
    Identifiable zoneID;
};

struct TreeRandomZoneTile : ZoneTile{
    
};

enum class BloatMode{
    STRAIGHT,
    RANDOM_DIAGONAL,
    RANDOM_TREE
};

template<typename T, typename EdgeType>
class ZoneBloater : public Simulator{
    public:
        //Empty grid tiles are considered NullID
        void initVoronoi(std::shared_ptr<const EdgeGraph<T, EdgeType>> graph, std::shared_ptr<Grid<T>> initialGrid);
        void onStart() override;
        void onStep() override;
        void onReset() override;
        void finishAndReset();
        std::shared_ptr<const EdgeGraph<T, EdgeType>> getGraph() const;
        std::shared_ptr<Grid<T>> getGrid() const;
        void straightVoronoiStep(std::shared_ptr<ZoneTile> activeTile);
        void diagonalRandomVoronoiStep(std::shared_ptr<ZoneTile> activeTile);
        BloatMode getMode() const{return bloatMode;};
        void setMode(BloatMode bloatMode);
    private:
        std::shared_ptr<const EdgeGraph<T, EdgeType>> graph;
        std::shared_ptr<Grid<T>> grid;
        std::queue<std::shared_ptr<ZoneTile>> nextExpanders;
        int max_expanders = 0;
        int currentStepSize = 0;
        //double diagonalChance = 0.4714045207;
        double diagonalChance = 0.4;
        BloatMode bloatMode = BloatMode::STRAIGHT;
        using AlgoFunc = void (ZoneBloater::*)(std::shared_ptr<ZoneTile>);
        AlgoFunc bloatStep = &ZoneBloater::straightVoronoiStep;
};

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::initVoronoi(std::shared_ptr<const EdgeGraph<T, EdgeType>> graph, std::shared_ptr<Grid<T>> initialGrid){
    this->graph = graph;
    grid = initialGrid;
    max_expanders = 4 * grid->getWidth() * grid->getHeight();
}

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::onStart(){
    if (!graph || !grid){
        finish();
        throw std::invalid_argument("No graph or grid are set!");
    }
    for (int y = 0; y < grid->getHeight(); y++){
        for (int x = 0; x < grid->getWidth(); x++){
            if (!grid->isEmpty(x, y)){
                switch (bloatMode){
                    case BloatMode::RANDOM_TREE:
                        break;
                    case BloatMode::RANDOM_DIAGONAL:
                    case BloatMode::STRAIGHT:
                    default:
                        nextExpanders.push(std::make_shared<ZoneTile>(ZoneTile(x, y, grid->getTileID(x, y))));
                }
                grid->setTile(x, y, Identifiable::nullID);
            }
        }
    }
}

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::onStep(){
    currentStepSize = nextExpanders.size();
    if (currentStepSize > max_expanders)
        throw std::logic_error("Size is more than grid\n");

    for (int i = 0; i < currentStepSize; i++){
        std::shared_ptr<ZoneTile> activeTile = nextExpanders.front();
        nextExpanders.pop();
        (this->*bloatStep)(activeTile);
    }
    if (nextExpanders.empty())
        finish();
}

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::onReset(){
    nextExpanders = std::queue<std::shared_ptr<ZoneTile>>();
}

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::finishAndReset(){
    if (isRunning())
        finish();
    if (isFinished())
        reset();
}

template <typename T, typename EdgeType>
std::shared_ptr<const EdgeGraph<T, EdgeType>> ZoneBloater<T, EdgeType>::getGraph() const{
    return graph;
}

template <typename T, typename EdgeType>
 std::shared_ptr<Grid<T>> ZoneBloater<T, EdgeType>::getGrid() const{
    return grid;
}

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::straightVoronoiStep(std::shared_ptr<ZoneTile> activeTile){
    int x = activeTile->x;
    int y = activeTile->y;
    if (!grid->isEmpty(x, y)){
        return;
    }

    grid->setTile(x, y, activeTile->zoneID);

    IntVector2 tempPoint = {x - 1, y};
    if (grid->isValidPoint(tempPoint))
        nextExpanders.push(std::make_shared<ZoneTile>(ZoneTile(tempPoint, activeTile->zoneID)));

    tempPoint = {x + 1, y};
    if (grid->isValidPoint(tempPoint))
        nextExpanders.push(std::make_shared<ZoneTile>(ZoneTile(tempPoint, activeTile->zoneID)));

    tempPoint = {x, y - 1};
    if (grid->isValidPoint(tempPoint))
        nextExpanders.push(std::make_shared<ZoneTile>(ZoneTile(tempPoint, activeTile->zoneID)));

    tempPoint = {x, y + 1};
    if (grid->isValidPoint(tempPoint))
        nextExpanders.push(std::make_shared<ZoneTile>(ZoneTile(tempPoint, activeTile->zoneID)));
}

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::diagonalRandomVoronoiStep(std::shared_ptr<ZoneTile> activeTile){
    int x = activeTile->x;
    int y = activeTile->y;
    if (!grid->isEmpty(x, y)){
        return;
    }
    straightVoronoiStep(activeTile);

    IntVector2 tempPoint = {x - 1, y - 1};
    if (grid->isValidPoint(tempPoint) && RandomGenerator::instance().chanceOccurred(diagonalChance))
        nextExpanders.push(std::make_shared<ZoneTile>(ZoneTile(tempPoint, activeTile->zoneID)));

    tempPoint = {x - 1, y + 1};
    if (grid->isValidPoint(tempPoint) && RandomGenerator::instance().chanceOccurred(diagonalChance))
        nextExpanders.push(std::make_shared<ZoneTile>(ZoneTile(tempPoint, activeTile->zoneID)));

    tempPoint = {x + 1, y + 1};
    if (grid->isValidPoint(tempPoint) && RandomGenerator::instance().chanceOccurred(diagonalChance))
        nextExpanders.push(std::make_shared<ZoneTile>(ZoneTile(tempPoint, activeTile->zoneID)));

    tempPoint = {x + 1, y - 1};
    if (grid->isValidPoint(tempPoint) && RandomGenerator::instance().chanceOccurred(diagonalChance))
        nextExpanders.push(std::make_shared<ZoneTile>(ZoneTile(tempPoint, activeTile->zoneID)));
}

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::setMode(BloatMode bloatMode){
    if (!isInitialized())
        throw std::logic_error("The status is not INIT");
    this->bloatMode = bloatMode;
    switch (bloatMode){
        case BloatMode::RANDOM_DIAGONAL:
            bloatStep = this->diagonalRandomVoronoiStep;
            break;
        case BloatMode::STRAIGHT:
        default:
            bloatStep = this->straightVoronoiStep;
            break;
    }
}
