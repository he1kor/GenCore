#pragma once

#include <memory>
#include <queue>

#include "grid.h"
#include "edge_graph.h"
#include "simulator.h"
#include "2d.h"
#include "identifiable.h"

struct ZoneTile : IntPoint2{
    ZoneTile(IntPoint2 point, Identifiable id) : IntPoint2{point.x, point.y}, zoneID(id){};
    ZoneTile(int x, int y, Identifiable id) : IntPoint2{x, y}, zoneID(id){};
    Identifiable zoneID;
};

template<typename T, typename EdgeType>
class ZoneBloater : public Simulator{
    public:
        //Empty grid tiles are considered NullID
        void initVoronoi(std::shared_ptr<const EdgeGraph<T, EdgeType>> graph, std::shared_ptr<Grid<T>> initialGrid);
        void onStart() override;
        void onStep() override;
        void onReset() override;
        std::shared_ptr<const EdgeGraph<T, EdgeType>> getGraph() const;
        std::shared_ptr<Grid<T>> getGrid() const;
    private:
        std::shared_ptr<const EdgeGraph<T, EdgeType>> graph;
        std::shared_ptr<Grid<T>> grid;
        std::queue<ZoneTile> nextExpanders;
        int max_expanders = 0;
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
                nextExpanders.push({x, y, grid->getTileID(x, y)});
                
                grid->setTile(x, y, Identifiable::nullID);
            } 
        }
    }
}

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::onStep(){
    int size = nextExpanders.size();
    if (size > max_expanders)
        throw std::logic_error("Size is more than grid\n");
    for (int i = 0; i < size; i++){
        ZoneTile activeTile = nextExpanders.front();
        nextExpanders.pop();

        int x = activeTile.x;
        int y = activeTile.y;
        if (!grid->isEmpty(x, y)){
            continue;
        }

        grid->setTile(x, y, activeTile.zoneID);

        IntPoint2 tempPoint = {x - 1, y};
        if (grid->isValidPoint(tempPoint))
            nextExpanders.push({tempPoint, activeTile.zoneID});

        tempPoint = {x + 1, y};
        if (grid->isValidPoint(tempPoint))
            nextExpanders.push({tempPoint, activeTile.zoneID});

        tempPoint = {x, y - 1};
        if (grid->isValidPoint(tempPoint))
            nextExpanders.push({tempPoint, activeTile.zoneID});

        tempPoint = {x, y + 1};
        if (grid->isValidPoint(tempPoint))
            nextExpanders.push({tempPoint,activeTile.zoneID});
    }
    if (nextExpanders.empty())
        finish();
}

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::onReset(){
    nextExpanders = std::queue<ZoneTile>();
}

template <typename T, typename EdgeType>
std::shared_ptr<const EdgeGraph<T, EdgeType>> ZoneBloater<T, EdgeType>::getGraph() const{
    return graph;
}

template <typename T, typename EdgeType>
 std::shared_ptr<Grid<T>> ZoneBloater<T, EdgeType>::getGrid() const{
    return grid;
}
