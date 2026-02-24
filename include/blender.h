#pragma once

#include <vector>
#include <queue>

#include "identifiable.h"
#include "matrix.h"
#include "grid.h"
#include "radial.h"
#include "edge_graph.h"
#include "connection.h"
#include "border.h"

template <typename T, typename SyncEdgeT>
std::unordered_map<Identifiable, Matrix<double>, IDHash> blendConnections(const Grid<T>& grid, const EdgeGraph<T, SyncEdgeT, BasicConnection>& mapTemplate);

template <typename T>
std::unordered_map<Identifiable, Matrix<double>, IDHash> buildZoneMasks(const Grid<T>& grid);

struct BFSIntakeElement{
    IntVector2 coords;
    Identifiable intakingZone;
    Identifiable spreadingZone;
    int iterationsLeft;
};

struct BFSBlendElement{
    IntVector2 coords;
    Identifiable spreadingZone;
    int blendDistance;
    int iterationsLeft;
};

template <typename T, typename SyncEdgeT>
inline std::unordered_map<Identifiable, Matrix<double>, IDHash> blendConnections(const Grid<T>& grid, const EdgeGraph<T, SyncEdgeT, BasicConnection>& mapTemplate)
{
    std::unordered_map<Identifiable, Matrix<double>, IDHash> result = buildZoneMasks(grid);
    std::unordered_map<std::pair<Identifiable, Identifiable>, std::vector<tiles::Border>, PairIDHash> borders = tiles::Border::getAllBorders(grid);
    std::queue<BFSIntakeElement> bfsQueue;
    const auto& asymEdges = mapTemplate.getAsymEdges();
    for (auto& [idPair, asymEdgeParams] : asymEdges){
        auto reversedIdPair = std::make_pair(idPair.second, idPair.first);
        auto it = borders.find(idPair);
        auto reversedIt = borders.find(reversedIdPair);
        if (it == borders.end()){
            it = reversedIt;
            if (reversedIt == borders.end()){
                continue; //No border found between these zones
            }
        }
        for (tiles::Border border : it->second){
            bool leftOriented = grid.getTileID(border.getLeft(0)) == idPair.first;
            for (tiles::Border::Segment segment : border.getSegments()){
                bfsQueue.push({
                    leftOriented? segment.getLeftPos() : segment.getRightPos(),
                    idPair.first,
                    idPair.second,
                    asymEdgeParams.intakeDistance
                });
            }
        }
    }
    while (!bfsQueue.empty()){
        BFSIntakeElement bfsElement = bfsQueue.front();
        bfsQueue.pop();
        if (!result.contains(bfsElement.intakingZone) || !result.contains(bfsElement.spreadingZone)){
            throw std::logic_error(std::format("NO ZONE ID FOUND: {} or {}", bfsElement.intakingZone.toString(), bfsElement.spreadingZone.toString()));
        }
        if (
            !grid.isValidPoint(bfsElement.coords)
            || result[bfsElement.intakingZone].get(bfsElement.coords) != 1.0
            || bfsElement.iterationsLeft < 1
        ){
            continue;
        }
        result[bfsElement.intakingZone].set(bfsElement.coords.x, bfsElement.coords.y, 0.0);
        result[bfsElement.spreadingZone].set(bfsElement.coords.x, bfsElement.coords.y, 1.0);
        bfsQueue.push({
            bfsElement.coords.x+1, bfsElement.coords.y,
            bfsElement.intakingZone,
            bfsElement.spreadingZone,
            bfsElement.iterationsLeft - 1
        });
        bfsQueue.push({
            bfsElement.coords.x-1, bfsElement.coords.y,
            bfsElement.intakingZone,
            bfsElement.spreadingZone,
            bfsElement.iterationsLeft - 1
        });
        bfsQueue.push({
            bfsElement.coords.x, bfsElement.coords.y+1,
            bfsElement.intakingZone,
            bfsElement.spreadingZone,
            bfsElement.iterationsLeft - 1
        });
        bfsQueue.push({ 
            bfsElement.coords.x, bfsElement.coords.y-1,
            bfsElement.intakingZone,
            bfsElement.spreadingZone,
            bfsElement.iterationsLeft - 1
        });
    }

    // Intake end, starting blend

    std::queue<BFSBlendElement> bfsBlendQueue;

    for (auto& [id, matrix] : result){    
        for (size_t y = 0; y < grid.getHeight(); ++y){
            for (size_t x = 0; x < grid.getWidth(); ++x){
                if (matrix.get(x, y) == 1.0) tryAddToBFSBlendQueue(bfsBlendQueue, IntVector2(x+1, y), id, result, asymEdges);
                if (matrix.get(x, y) == 1.0) tryAddToBFSBlendQueue(bfsBlendQueue, IntVector2(x-1, y), id, result, asymEdges);
                if (matrix.get(x, y) == 1.0) tryAddToBFSBlendQueue(bfsBlendQueue, IntVector2(x, y+1), id, result, asymEdges);
                if (matrix.get(x, y) == 1.0) tryAddToBFSBlendQueue(bfsBlendQueue, IntVector2(x, y-1), id, result, asymEdges);
            }
        }
    }

    Matrix<std::unordered_map<Identifiable,double, IDHash>> tileZoneInfluence(grid.getWidth(), grid.getHeight()); // Each tile DOESN'T have all the zones, so need to check
    while (!bfsBlendQueue.empty()){
        BFSBlendElement bfsElement = bfsBlendQueue.front();
        bfsBlendQueue.pop();
        if (
            !grid.isValidPoint(bfsElement.coords)
            || grid.getTileID(bfsElement.coords) == Identifiable::nullID
            || bfsElement.iterationsLeft < 1
        ){
            continue;
        }
        
        double influence = (bfsElement.iterationsLeft + 1.0) / (bfsElement.blendDistance + 1.0);

        if (tileZoneInfluence.get(bfsElement.coords).contains(bfsElement.spreadingZone) && tileZoneInfluence.get(bfsElement.coords).at(bfsElement.spreadingZone) >= influence){
            continue;
        }
    
        if (result.at(bfsElement.spreadingZone).get(bfsElement.coords) == 1.0){
            tileZoneInfluence.access(bfsElement.coords)[bfsElement.spreadingZone] = 1.0;
        } else{
            tileZoneInfluence.access(bfsElement.coords)[bfsElement.spreadingZone] = influence;
        }

        bfsBlendQueue.push({
            bfsElement.coords.x+1, bfsElement.coords.y,
            bfsElement.spreadingZone,
            bfsElement.blendDistance,
            bfsElement.iterationsLeft - 1
        });
        bfsBlendQueue.push({
            bfsElement.coords.x-1, bfsElement.coords.y,
            bfsElement.spreadingZone,
            bfsElement.blendDistance,
            bfsElement.iterationsLeft - 1
        });
        bfsBlendQueue.push({
            bfsElement.coords.x, bfsElement.coords.y+1,
            bfsElement.spreadingZone,
            bfsElement.blendDistance,
            bfsElement.iterationsLeft - 1
        });
        bfsBlendQueue.push({ 
            bfsElement.coords.x, bfsElement.coords.y-1,
            bfsElement.spreadingZone,
            bfsElement.blendDistance,
            bfsElement.iterationsLeft - 1
        });
    }

    for (auto& [id, matrix] : result){    
        for (size_t y = 0; y < grid.getHeight(); ++y){
            for (size_t x = 0; x < grid.getWidth(); ++x){
                if (matrix.get(x, y) == 1.0 && !tileZoneInfluence.get(x, y).contains(id)){
                    tileZoneInfluence.access(x, y)[id] = 1.0;
                }
                if (!tileZoneInfluence.get(x, y).contains(id)){
                    continue;
                }

;
                double sum = std::accumulate(
                    tileZoneInfluence.get(x, y).begin(), 
                    tileZoneInfluence.get(x, y).end(), 
                    0.0,
                    [](double acc, const auto& pair) {
                        return acc + pair.second;
                    }
                );
                result[id].set(x, y, tileZoneInfluence.get(x, y).at(id) / sum);
                // if (id.getID() == 4){
                    // std::cout << std::format("Result at [{}; {}] is {} for influence {}, sum calculated was: {}\n", x, y, result[id].get(x, y), tileZoneInfluence.get(x, y).at(id), sum);
                // }
            }
        }
    }

    return result;
}

void tryAddToBFSBlendQueue(
        std::queue<BFSBlendElement>& bfsBlendQueue,
        IntVector2 coords,
        Identifiable spreadingZone,
        const std::unordered_map<Identifiable, Matrix<double>, IDHash>& matrices,
        const std::unordered_map<std::pair<Identifiable, Identifiable>, BasicConnection, AsymPairIDHash>& asymEdges
    ){
    if (
        !matrices.at(spreadingZone).isValidPoint(coords)
        || matrices.at(spreadingZone).get(coords) == 1.0
    ){
        return;
    }
    for (auto& [zoneID, matrix] : matrices){
        if (
            zoneID == spreadingZone
            || matrix.get(coords) != 1.0
        ){
            continue;
        }
        auto it = asymEdges.find({zoneID, spreadingZone});
        if (it == asymEdges.end()){
            it = asymEdges.find({spreadingZone, zoneID});
        }
        if (it == asymEdges.end()){
            continue; //actually should be the error
            throw std::logic_error(std::format("No connection between {} and {}, but found open border at {}", zoneID.toString(), spreadingZone.toString(), coords.toString()));
        }
        bfsBlendQueue.push({
            coords,
            zoneID,
            it->second.blendDistance,
            it->second.blendDistance
        });
    }
}


template <typename T>
inline std::unordered_map<Identifiable, Matrix<double>, IDHash> buildZoneMasks(const Grid<T>& grid)
{
    std::unordered_map<Identifiable, Matrix<double>, IDHash> result;
    for (Identifiable id : grid.getTileIDs()){
        result[id] = Matrix<double>(grid.getWidth(), grid.getHeight());       
    }
    for (auto it = grid.begin(); it != grid.end(); ++it){
        if (*it == Identifiable::nullID){
            continue;
        }
        result[*it].set(it.getX(), it.getY(), 1.0);
    }
    return result;
}
