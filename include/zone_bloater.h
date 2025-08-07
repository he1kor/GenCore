#pragma once

#include <memory>
#include <queue>


#include "grid.h"
#include "edge_graph.h"
#include "simulator.h"
#include "2d.h"
#include "identifiable.h"
#include "random_generator.h"
#include "radial.h"
#include "line.h"

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

template<typename T, typename EdgeType = Identifiable>
class ZoneBloater : public Simulator{
    public:
        //Empty grid tiles are considered NullID
        void initVoronoi(std::shared_ptr<const EdgeGraph<T, EdgeType>> graph, std::shared_ptr<Grid<T>> initialGrid);
        virtual void onStart() override;
        virtual void onStep() override;
        void onReset() override;
        void finishAndReset();

        std::shared_ptr<const EdgeGraph<T, EdgeType>> getGraph() const;
        std::shared_ptr<Grid<T>> getGrid() const;

        void straightVoronoiStep(std::shared_ptr<ZoneTile> activeTile);
        void diagonalRandomVoronoiStep(std::shared_ptr<ZoneTile> activeTile);

        BloatMode getMode() const{return bloatMode;};
        virtual void setMode(BloatMode bloatMode);

        void setStartFromEdges(bool value);
        bool getStartFromEdges() const {return startFromEdges;};

    private:

        std::shared_ptr<const EdgeGraph<T, EdgeType>> graph;
        std::shared_ptr<Grid<T>> grid;
        std::unordered_map<Identifiable, IntVector2, IDHash> startingPoints;
        std::queue<std::shared_ptr<ZoneTile>> nextExpanders;
        int max_expanders = 0;
        int currentStepSize = 0;
        //double diagonalChance = 0.4714045207;
        double diagonalChance = 0.4;

        BloatMode bloatMode = BloatMode::STRAIGHT;
        using AlgoFunc = void (ZoneBloater::*)(std::shared_ptr<ZoneTile>);
        AlgoFunc bloatStep = &ZoneBloater::straightVoronoiStep;

        bool startFromEdges = false;

    private:
        
        void setEdgeExpanders();
};

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::initVoronoi(std::shared_ptr<const EdgeGraph<T, EdgeType>> graph, std::shared_ptr<Grid<T>> initialGrid){
    this->graph = graph;
    grid = initialGrid;
    startingPoints.clear();
    for (size_t y = 0; y < grid->getHeight(); y++){
        for (size_t x = 0; x < grid->getWidth(); x++){
            if (!grid->isEmpty(x, y)){
                startingPoints[grid->getTileID(x, y)] = {static_cast<int>(x), static_cast<int>(y)};
            }
        }
    }
    max_expanders = 4 * grid->getWidth() * grid->getHeight();
}

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::onStart(){
    if (!graph || !grid){
        finish();
        throw std::invalid_argument("No graph or grid are set!");
    }
    for (const auto& [startingTileID, vector2] : startingPoints){
        auto [x, y] = vector2;
        switch (bloatMode){
            case BloatMode::RANDOM_TREE:
                break;
            case BloatMode::RANDOM_DIAGONAL:
            case BloatMode::STRAIGHT:
            default:
                nextExpanders.push(std::make_shared<ZoneTile>(ZoneTile(x, y, startingTileID)));
        }
        grid->setTile(x, y, Identifiable::nullID);
    }
    if (startFromEdges)
        setEdgeExpanders();
}

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::onStep(){
    currentStepSize = nextExpanders.size();
    if (currentStepSize > max_expanders)
        throw std::logic_error("Size is more than grid:\t" + std::to_string(currentStepSize) + "\n");

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

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::setStartFromEdges(bool value){
    startFromEdges = value;
}

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::setEdgeExpanders(){
    double ratio = 0.5;
    
    for (const Identifiable& edgeID : graph->getEdgeIDs()){
        auto [startPointID1, startPointID2] = graph->getEdgeNodeIDs(edgeID);
        RasterLine line(
            startingPoints.at(startPointID1),
            startingPoints.at(startPointID2)
        );

        if constexpr (std::is_base_of_v<Radial, T>){
            double radius1 = static_cast<Radial>(graph->getValue(startPointID1)).getRadius();
            double radius2 = static_cast<Radial>(graph->getValue(startPointID2)).getRadius();
            ratio = radius1 / (radius1 + radius2);
        }
        size_t upperBound = static_cast<size_t>(ratio * line.size());
        for (size_t i = 0; i < upperBound; i++){
            nextExpanders.push(std::make_shared<ZoneTile>(line[i], startPointID1));
        }
        for (size_t i = upperBound; i < line.size(); i++){
            nextExpanders.push(std::make_shared<ZoneTile>(line[i], startPointID2));
        }
    }
}
