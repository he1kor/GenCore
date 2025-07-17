#pragma once

#include <memory>
#include <queue>

#include "grid.h"
#include "edge_graph.h"
#include "simulator.h"
#include "2d.h"

struct ZoneTile : IntPoint2{
    ZoneTile(IntPoint2 point, Identifiable id) : IntPoint2(point.x, point.y), zoneID(id){};
    Identifiable zoneID;
};

template<typename T, typename EdgeType>
class ZoneBloater : Simulator{
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
};

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::initVoronoi(std::shared_ptr<const EdgeGraph<T, EdgeType>> graph, std::shared_ptr<Grid<T>> initialGrid){
    this->graph = graph;
    grid = initialGrid;
}

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::onStart(){
    if (!graph || !grid){
        finish();
        throw std::exception("No graph or grid are set!");
    }
    for (int y = 0; y < grid->getHeight(); y++){
        for (int x = 0; x < grid->getWidth(); x++){
            if (!grid->isEmpty(x, y)){
                nextExpanders.push({x, y, grid->getID(x, y)});
            } 
        }
    }
}

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::onStep(){
    for (int i = 0; i < nextExpanders.size(); i++){
        ZoneTile activeTile = nextExpanders.front();
        nextExpanders.pop();

        int x = activeTile.x;
        int y = activeTile.y;
        if (!grid->isEmpty(x, y))
            continue;
        grid->setTile(x, y, activeTile.zoneID);

        IntPoint2 tempPoint = {x - 1, y};
        if (auto id = grid->tryGetID(tempPoint))
            nextExpanders.push({tempPoint, id});

        tempPoint = {x + 1, y};
        if (auto id = grid->tryGetID(tempPoint))
            nextExpanders.push({tempPoint, id});

        tempPoint = {x, y - 1};
        if (auto id = grid->tryGetID(tempPoint))
            nextExpanders.push({tempPoint, id});

        tempPoint = {x, y + 1};
        if (auto id = grid->tryGetID(tempPoint))
            nextExpanders.push({tempPoint, id});
    }
    if (nextExpanders.empty())
        finish();
}

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::onReset(){
    nextExpanders = std::queue<IntPoint2>();
}

template <typename T, typename EdgeType>
std::shared_ptr<const EdgeGraph<T, EdgeType>> ZoneBloater<T, EdgeType>::getGraph() const{
    return graph;
}

template <typename T, typename EdgeType>
 std::shared_ptr<Grid<T>> ZoneBloater<T, EdgeType>::getGrid() const{
    return grid;
}
