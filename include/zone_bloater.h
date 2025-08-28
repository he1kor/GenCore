#pragma once

#include <memory>
#include <queue>

#include "self_pointer.h"

#include "grid.h"
#include "edge_graph.h"
#include "simulator.h"
#include "2d.h"
#include "identifiable.h"
#include "random_generator.h"
#include "radial.h"
#include "line.h"
#include "bloat_strategy.h"

template<typename T, typename EdgeType = Identifiable>
class ZoneBloater : public Simulator, public ZoneTilePusher{
    public:
        //Empty grid tiles are considered NullID
        void initVoronoi(std::shared_ptr<const EdgeGraph<T, EdgeType>> graph, std::shared_ptr<Grid<T>> initialGrid);
        virtual void onStart() override;
        virtual void onStep() override;
        void onReset() override;
        void finishAndReset();

        std::shared_ptr<const EdgeGraph<T, EdgeType>> getGraph() const;
        std::shared_ptr<Grid<T>> getGrid() const;

        template <typename Strategy>
        requires std::is_base_of_v<BloatStrategy, std::decay_t<Strategy>>
        void setBloatMode(Strategy&& bloatStrategy);
        void setStartFromEdges(bool value);
        bool getStartFromEdges() const {return startFromEdges;};

    protected:
        void push(const ZoneTile& zoneTile) override;
        void setZoneTile(const ZoneTile& zoneTile) override;
        bool isEmpty(IntVector2 point) const override;
        bool isValidPoint(IntVector2 point) const override;
    private:

        std::shared_ptr<const EdgeGraph<T, EdgeType>> graph;
        std::shared_ptr<Grid<T>> grid;
        std::unordered_map<Identifiable, IntVector2, IDHash> startingPoints;
        std::queue<std::shared_ptr<ZoneTile>> nextExpanders;
        int max_expanders = 0;
        int currentStepSize = 0;

        bool startFromEdges = false;

        std::unique_ptr<BloatStrategy> bloatStrategy;

    private:
        
        void setEdgeExpanders();
};

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::initVoronoi(std::shared_ptr<const EdgeGraph<T, EdgeType>> graph, std::shared_ptr<Grid<T>> initialGrid){
    this->graph = graph;
    grid = initialGrid;
    startingPoints.clear();
    for (auto it = grid->begin(); it != grid->end(); ++it){
        if (*it != Identifiable::nullID){
            startingPoints[*it] = {it.getX(), it.getY()};
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
        nextExpanders.push(std::make_shared<ZoneTile>(ZoneTile(x, y, startingTileID)));
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
        bloatStrategy->bloat(*activeTile);
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
template <typename Strategy>
requires std::is_base_of_v<BloatStrategy, std::decay_t<Strategy>>
void ZoneBloater<T, EdgeType>::setBloatMode(Strategy&& bloatStrategy){
    if (!isInitialized())
        throw std::logic_error("The status is not INIT");
    this->bloatStrategy = std::make_unique<std::decay_t<Strategy>>(std::forward<Strategy>(bloatStrategy));
    this->bloatStrategy->setZoneTilePusher(this->self());
}

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::setStartFromEdges(bool value)
{
    startFromEdges = value;
}

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::push(const ZoneTile& zoneTile){
    nextExpanders.push(std::make_shared<ZoneTile>(zoneTile.x, zoneTile.y, zoneTile.zoneID));
}

template <typename T, typename EdgeType>
void ZoneBloater<T, EdgeType>::setZoneTile(const ZoneTile &zoneTile){
    grid->setTile(zoneTile.x, zoneTile.y, zoneTile.zoneID);
}

template <typename T, typename EdgeType>
bool ZoneBloater<T, EdgeType>::isEmpty(IntVector2 point) const{
    return grid->isEmpty(point);
}

template <typename T, typename EdgeType>
bool ZoneBloater<T, EdgeType>::isValidPoint(IntVector2 point) const{
    return grid->isValidPoint(point);
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
