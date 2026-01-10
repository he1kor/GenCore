#pragma once

#include <memory>
#include <queue>

#include "self_pointer.h"

#include "identifiable.h"
#include "grid.h"
#include "edge_graph.h"
#include "simulator.h"
#include "2d.h"
#include "identifiable.h"
#include "random_generator.h"
#include "radial.h"
#include "line.h"
#include "bloat_strategy.h"

template<typename T, typename SymEdgeT, typename AsymEdgeT>
class ZoneBloater : public Simulator, public ZoneTilePusher{
    public:
        //Empty grid tiles are considered NullID
        void initEdgeVoronoi(const EdgeGraph<T, SymEdgeT, AsymEdgeT>& graph, std::shared_ptr<Grid<T>> initialGrid);
        void initVoronoi(std::shared_ptr<Grid<T>> initialGrid);
        void initAdjacentCornerFill(std::shared_ptr<Grid<T>> grid);
        virtual void onStart() override;
        virtual void onStep() override;
        void onReset() override;
        void finishAndReset();

        std::shared_ptr<Grid<T>> getGrid() const;

        template <typename Strategy>
        requires std::is_base_of_v<BloatStrategy, std::decay_t<Strategy>>
        void setBloatMode(Strategy&& bloatStrategy);

    protected:
        void push(const ZoneTile& zoneTile) override;
        void setZoneTile(const ZoneTile& zoneTile) override;
        bool isEmpty(IntVector2 point) const override;
        bool isValidPoint(IntVector2 point) const override;
        std::optional<Identifiable> tryGetID(IntVector2 point) const override;
    private:

        std::shared_ptr<Grid<T>> grid;
        
        std::queue<std::shared_ptr<ZoneTile>> nextExpanders;
        int max_expanders = 0;
        int currentStepSize = 0;

        std::unique_ptr<BloatStrategy> bloatStrategy;

    private:
        void setEdgeExpanders(const std::unordered_map<Identifiable, IntVector2, IDHash>& startingPoints, const EdgeGraph<T, SymEdgeT, AsymEdgeT>& graph);
};

template <typename T, typename SymEdgeT, typename AsymEdgeT>
void ZoneBloater<T, SymEdgeT, AsymEdgeT>::initEdgeVoronoi(const EdgeGraph<T, SymEdgeT, AsymEdgeT>& graph, std::shared_ptr<Grid<T>> initialGrid){
    grid = initialGrid;
    std::unordered_map<Identifiable, IntVector2, IDHash> startingPoints;
    for (auto it = grid->begin(); it != grid->end(); ++it){
        if (*it != Identifiable::nullID){
            startingPoints[*it] = {it.getX(), it.getY()};
            grid->setTile(it.getX(), it.getY(), Identifiable::nullID);
        }
    }
    setEdgeExpanders(startingPoints, graph);
    max_expanders = 8 * grid->getWidth() * grid->getHeight();
}

template <typename T, typename SymEdgeT, typename AsymEdgeT>
void ZoneBloater<T, SymEdgeT, AsymEdgeT>::initVoronoi(std::shared_ptr<Grid<T>> initialGrid){
    grid = initialGrid;
    for (auto it = grid->begin(); it != grid->end(); ++it){
        if (*it != Identifiable::nullID){
            grid->setTile(it.getX(), it.getY(), Identifiable::nullID);
            nextExpanders.push(std::make_shared<ZoneTile>(ZoneTile(it.getX(), it.getY(), *it)));
        }
    }
    max_expanders = 8 * grid->getWidth() * grid->getHeight();
}

template <typename T, typename SymEdgeT, typename AsymEdgeT>
void ZoneBloater<T, SymEdgeT, AsymEdgeT>::initAdjacentCornerFill(std::shared_ptr<Grid<T>> grid){
    setBloatMode(bloatStrategies::AdjacentCornerFill());
    this->grid = grid;
    for (auto it = grid->begin(); it != grid->end(); ++it){
        if (*it == Identifiable::nullID){
            nextExpanders.push(std::make_shared<ZoneTile>(it.getX(), it.getY(), Identifiable::nullID));
        }
    }
    max_expanders = 4 * grid->getWidth() * grid->getHeight();
}

template <typename T, typename SymEdgeT, typename AsymEdgeT>
void ZoneBloater<T, SymEdgeT, AsymEdgeT>::onStart(){
    if (!grid){
        finish();
        throw std::invalid_argument("No grid is set!");
    }
}

template <typename T, typename SymEdgeT, typename AsymEdgeT>
void ZoneBloater<T, SymEdgeT, AsymEdgeT>::onStep(){
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

template <typename T, typename SymEdgeT, typename AsymEdgeT>
void ZoneBloater<T, SymEdgeT, AsymEdgeT>::onReset(){
    nextExpanders = std::queue<std::shared_ptr<ZoneTile>>();
}

template <typename T, typename SymEdgeT, typename AsymEdgeT>
void ZoneBloater<T, SymEdgeT, AsymEdgeT>::finishAndReset(){
    if (isRunning())
        finish();
    if (isFinished())
        reset();
}

template <typename T, typename SymEdgeT, typename AsymEdgeT>
std::shared_ptr<Grid<T>> ZoneBloater<T, SymEdgeT, AsymEdgeT>::getGrid() const{
    return grid;
}

template <typename T, typename SymEdgeT, typename AsymEdgeT>
template <typename Strategy>
requires std::is_base_of_v<BloatStrategy, std::decay_t<Strategy>>
void ZoneBloater<T, SymEdgeT, AsymEdgeT>::setBloatMode(Strategy&& bloatStrategy){
    if (!isInitialized())
        throw std::logic_error("The status is not INIT");
    this->bloatStrategy = std::make_unique<std::decay_t<Strategy>>(std::forward<Strategy>(bloatStrategy));
    this->bloatStrategy->setZoneTilePusher(this->self());
}

template <typename T, typename SymEdgeT, typename AsymEdgeT>
void ZoneBloater<T, SymEdgeT, AsymEdgeT>::push(const ZoneTile& zoneTile){
    nextExpanders.push(std::make_shared<ZoneTile>(zoneTile.x, zoneTile.y, zoneTile.zoneID));
}

template <typename T, typename SymEdgeT, typename AsymEdgeT>
void ZoneBloater<T, SymEdgeT, AsymEdgeT>::setZoneTile(const ZoneTile &zoneTile){
    grid->setTile(zoneTile.x, zoneTile.y, zoneTile.zoneID);
}

template <typename T, typename SymEdgeT, typename AsymEdgeT>
bool ZoneBloater<T, SymEdgeT, AsymEdgeT>::isEmpty(IntVector2 point) const{
    return grid->isEmpty(point);
}

template <typename T, typename SymEdgeT, typename AsymEdgeT>
bool ZoneBloater<T, SymEdgeT, AsymEdgeT>::isValidPoint(IntVector2 point) const{
    return grid->isValidPoint(point);
}

template <typename T, typename SymEdgeT, typename AsymEdgeT>
std::optional<Identifiable> ZoneBloater<T, SymEdgeT, AsymEdgeT>::tryGetID(IntVector2 point) const{
    return grid->tryGetID(point);
}

template <typename T, typename SymEdgeT, typename AsymEdgeT>
void ZoneBloater<T, SymEdgeT, AsymEdgeT>::setEdgeExpanders(const std::unordered_map<Identifiable, IntVector2, IDHash>& startingPoints, const EdgeGraph<T, SymEdgeT, AsymEdgeT>& graph){
    double ratio = 0.5;
    
    for (const auto& [nodePair, edge] : graph.getSymEdges()){
        RasterLine line(
            startingPoints.at(nodePair.first),
            startingPoints.at(nodePair.second)
        );

        if constexpr (std::is_base_of_v<Radial, T>){
            double radius1 = static_cast<Radial>(graph.getValue(nodePair.first)).getRadius();
            double radius2 = static_cast<Radial>(graph.getValue(nodePair.second)).getRadius();
            ratio = radius1 / (radius1 + radius2);
        }
        size_t upperBound = static_cast<size_t>(ratio * line.size());
        for (size_t i = 0; i < upperBound; i++){
            nextExpanders.push(std::make_shared<ZoneTile>(line[i], nodePair.first));
        }
        for (size_t i = upperBound; i < line.size(); i++){
            nextExpanders.push(std::make_shared<ZoneTile>(line[i], nodePair.second));
        }
    }
}
