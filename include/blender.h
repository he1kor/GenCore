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

struct ZoneMasks{
    // Used to determine increased/decreased power of tiles, for example for resources.
    std::unordered_map<Identifiable, Matrix<double>, IDHash> bonusMask;

     // Used to determine how much imapct this zone has on the tile. Each tile is supposed to have 0.0 or 1.0 sum of all zones influences.
    std::unordered_map<Identifiable, Matrix<double>, IDHash> influenceMask;
};

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

struct BFSGuarantorElement{
    IntVector2 coords;
    Identifiable zoneApplied;
    Identifiable neighbourStarted;
    double weightSpread;
};

template <typename T, typename SyncEdgeT>
ZoneMasks blendConnections(const Grid<T>& grid, const EdgeGraph<T, SyncEdgeT, BasicConnection>& mapTemplate);

void tryAddToBFSBlendQueue(
        std::queue<BFSBlendElement>& bfsBlendQueue,
        IntVector2 coords,
        Identifiable spreadingZone,
        const std::unordered_map<Identifiable, Matrix<double>, IDHash>& matrices,
        const std::unordered_map<std::pair<Identifiable, Identifiable>, BasicConnection, AsymPairIDHash>& asymEdges
    );

template <typename T>
std::unordered_map<Identifiable, Matrix<double>, IDHash> buildZoneMasks(const Grid<T>& grid);



template <typename T, typename SyncEdgeT>
inline ZoneMasks blendConnections(const Grid<T>& grid, const EdgeGraph<T, SyncEdgeT, BasicConnection>& mapTemplate)
{
    std::unordered_map<Identifiable, Matrix<double>, IDHash> zoneInfluence = buildZoneMasks(grid);

    std::unordered_map<std::pair<Identifiable, Identifiable>, std::vector<tiles::Border>, PairIDHash> borders = tiles::Border::getAllBorders(grid);
    std::queue<BFSIntakeElement> bfsIntakeQueue;
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
                bfsIntakeQueue.push({
                    leftOriented? segment.getLeftPos() : segment.getRightPos(),
                    idPair.first,
                    idPair.second,
                    asymEdgeParams.intakeDistance
                });
            }
        }
    }

    while (!bfsIntakeQueue.empty()){
        BFSIntakeElement bfsElement = bfsIntakeQueue.front();
        bfsIntakeQueue.pop();
        if (!zoneInfluence.contains(bfsElement.intakingZone) || !zoneInfluence.contains(bfsElement.spreadingZone)){
            throw std::logic_error(std::format("NO ZONE ID FOUND: {} or {}", bfsElement.intakingZone.toString(), bfsElement.spreadingZone.toString()));
        }
        if (
            !grid.isValidPoint(bfsElement.coords)
            || zoneInfluence.at(bfsElement.intakingZone).get(bfsElement.coords) != 1.0
            || bfsElement.iterationsLeft < 1
        ){
            continue;
        }
        zoneInfluence.at(bfsElement.intakingZone).set(bfsElement.coords.x, bfsElement.coords.y, 0.0);
        zoneInfluence.at(bfsElement.spreadingZone).set(bfsElement.coords.x, bfsElement.coords.y, 1.0);
        bfsIntakeQueue.push({
            bfsElement.coords.x+1, bfsElement.coords.y,
            bfsElement.intakingZone,
            bfsElement.spreadingZone,
            bfsElement.iterationsLeft - 1
        });
        bfsIntakeQueue.push({
            bfsElement.coords.x-1, bfsElement.coords.y,
            bfsElement.intakingZone,
            bfsElement.spreadingZone,
            bfsElement.iterationsLeft - 1
        });
        bfsIntakeQueue.push({
            bfsElement.coords.x, bfsElement.coords.y+1,
            bfsElement.intakingZone,
            bfsElement.spreadingZone,
            bfsElement.iterationsLeft - 1
        });
        bfsIntakeQueue.push({ 
            bfsElement.coords.x, bfsElement.coords.y-1,
            bfsElement.intakingZone,
            bfsElement.spreadingZone,
            bfsElement.iterationsLeft - 1
        });
    }
    // Intake end, starting guarantor

    std::unordered_map<Identifiable, Matrix<double>, IDHash> zoneBonuses;
    for (Identifiable zone : mapTemplate.getIDs()){
        zoneBonuses[zone] = Matrix(grid.getWidth(), grid.getHeight(), 0.0);

    }
    std::queue<BFSGuarantorElement> bfsGuarantorQueue;
    std::unordered_map<std::pair<Identifiable, Identifiable>, int, AsymPairIDHash> areaLeft;

    for (auto& [idPair, asymEdgeParams] : asymEdges){
        areaLeft[idPair] = asymEdgeParams.areaGuaranteed;
    }

    for (auto& [id, matrix] : zoneInfluence){
        for (size_t y = 0; y < grid.getHeight(); ++y){
            for (size_t x = 0; x < grid.getWidth(); ++x){
                if (zoneInfluence.at(id).get(x, y) < 1.0){
                    continue;
                }
                tryAddToBFSGuarantorQueue(bfsGuarantorQueue, IntVector2(x+1, y), id, zoneBonuses, zoneInfluence, asymEdges);
                tryAddToBFSGuarantorQueue(bfsGuarantorQueue, IntVector2(x-1, y), id, zoneBonuses, zoneInfluence, asymEdges);
                tryAddToBFSGuarantorQueue(bfsGuarantorQueue, IntVector2(x, y+1), id, zoneBonuses, zoneInfluence, asymEdges);
                tryAddToBFSGuarantorQueue(bfsGuarantorQueue, IntVector2(x, y-1), id, zoneBonuses, zoneInfluence, asymEdges);
            }
        }
    }

    while (!bfsGuarantorQueue.empty()){
        BFSGuarantorElement bfsElement = bfsGuarantorQueue.front();
        bfsGuarantorQueue.pop();

        if (
            !grid.isValidPoint(bfsElement.coords)
            || zoneInfluence.at(bfsElement.zoneApplied).get(bfsElement.coords) <= 0.0
            || zoneBonuses.at(bfsElement.zoneApplied).get(bfsElement.coords) >= bfsElement.weightSpread
            || areaLeft.at(std::make_pair(bfsElement.zoneApplied, bfsElement.neighbourStarted)) < 1
        ){
            continue;
        }

        zoneBonuses.at(bfsElement.zoneApplied).access(bfsElement.coords) += bfsElement.weightSpread;
        areaLeft.at(std::make_pair(bfsElement.zoneApplied, bfsElement.neighbourStarted))--;

        bfsGuarantorQueue.push(BFSGuarantorElement{
            .coords = {bfsElement.coords.x-1, bfsElement.coords.y},
            .zoneApplied = bfsElement.zoneApplied,
            .neighbourStarted = bfsElement.neighbourStarted,
            .weightSpread = bfsElement.weightSpread
        });
                bfsGuarantorQueue.push(BFSGuarantorElement{
            .coords = {bfsElement.coords.x+1, bfsElement.coords.y},
            .zoneApplied = bfsElement.zoneApplied,
            .neighbourStarted = bfsElement.neighbourStarted,
            .weightSpread = bfsElement.weightSpread
        });
                bfsGuarantorQueue.push(BFSGuarantorElement{
            .coords = {bfsElement.coords.x, bfsElement.coords.y+1},
            .zoneApplied = bfsElement.zoneApplied,
            .neighbourStarted = bfsElement.neighbourStarted,
            .weightSpread = bfsElement.weightSpread
        });
                bfsGuarantorQueue.push(BFSGuarantorElement{
            .coords = {bfsElement.coords.x, bfsElement.coords.y+1},
            .zoneApplied = bfsElement.zoneApplied,
            .neighbourStarted = bfsElement.neighbourStarted,
            .weightSpread = bfsElement.weightSpread
        });
    }


    // Guarantor end, starting blend
    std::queue<BFSBlendElement> bfsBlendQueue;

    for (auto& [id, matrix] : zoneInfluence){    
        for (size_t y = 0; y < grid.getHeight(); ++y){
            for (size_t x = 0; x < grid.getWidth(); ++x){
                if (matrix.get(x, y) <= 0.001){
                    continue;
                }
                tryAddToBFSBlendQueue(bfsBlendQueue, IntVector2(x+1, y), id, zoneInfluence, asymEdges);
                tryAddToBFSBlendQueue(bfsBlendQueue, IntVector2(x-1, y), id, zoneInfluence, asymEdges);
                tryAddToBFSBlendQueue(bfsBlendQueue, IntVector2(x, y+1), id, zoneInfluence, asymEdges);
                tryAddToBFSBlendQueue(bfsBlendQueue, IntVector2(x, y-1), id, zoneInfluence, asymEdges);
            }
        }
    }

    std::unordered_map<Identifiable, Matrix<double>, IDHash> zoneBlendInfluence;
    for (Identifiable id : mapTemplate.getIDs()){
        zoneBlendInfluence[id] = Matrix(grid.getWidth(), grid.getHeight(), 0.0);
    }
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
        if (zoneBlendInfluence.at(bfsElement.spreadingZone).get(bfsElement.coords) >= influence){
            continue;
        }
        if (zoneInfluence.at(bfsElement.spreadingZone).get(bfsElement.coords) >= 1.0){
            zoneBlendInfluence.at(bfsElement.spreadingZone).access(bfsElement.coords) = 1.0;
        } else{
            zoneBlendInfluence.at(bfsElement.spreadingZone).access(bfsElement.coords) = influence;
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

    Matrix<double> sumInfluenceMatrix = Matrix(grid.getWidth(), grid.getHeight(), 0.0);
    for (auto& [id, matrix] : zoneBlendInfluence){    
        for (size_t y = 0; y < grid.getHeight(); ++y){
            for (size_t x = 0; x < grid.getWidth(); ++x){
                if (matrix.get(x, y) == 0.0 && zoneInfluence.at(id).get(x, y) >= 1.0){
                    matrix.set(x, y, 1.0);
                }
                sumInfluenceMatrix.access(x, y) += matrix.get(x, y);
            }
        }
    }

    for (auto& [id, matrix] : zoneInfluence){    
        for (size_t y = 0; y < grid.getHeight(); ++y){
            for (size_t x = 0; x < grid.getWidth(); ++x){            
                if (sumInfluenceMatrix.get(x, y) <= 0.0001){
                    matrix.set(x, y, 0.0);
                    continue;
                }    
                matrix.access(x, y) = zoneBlendInfluence.at(id).get(x, y) / sumInfluenceMatrix.get(x, y);
            }
        }
    }

    return ZoneMasks{
        .bonusMask = zoneBonuses,
        .influenceMask = zoneInfluence
    };
}


inline void tryAddToBFSGuarantorQueue(
        std::queue<BFSGuarantorElement>& bfsGuarantorQueue,
        IntVector2 coords,
        Identifiable neighbourZone,
        const std::unordered_map<Identifiable, Matrix<double>, IDHash>& matrices,
        const std::unordered_map<Identifiable, Matrix<double>, IDHash>& zoneMasks,
        const std::unordered_map<std::pair<Identifiable, Identifiable>, BasicConnection, AsymPairIDHash>& asymEdges
    ){
    if (
        !matrices.at(neighbourZone).isValidPoint(coords)
        || zoneMasks.at(neighbourZone).get(coords) >= 0.001
    ){
        return;
    }
    for (auto& [spreadingZone, matrix] : matrices){
        if (
            spreadingZone == neighbourZone
            || zoneMasks.at(spreadingZone).get(coords) <= 0.001
        ){
            continue; // We seek for neighbour DIFFERENT zone
        }
        auto it = asymEdges.find({spreadingZone, neighbourZone}); // Main is where guaranteed area is applied. Neighbour is the relative border where it start from.
        if (it == asymEdges.end()){
            continue;
        }
        bfsGuarantorQueue.push(BFSGuarantorElement{
            .coords = coords,
            .zoneApplied = spreadingZone,
            .neighbourStarted = neighbourZone,
            .weightSpread = it->second.bonusValue
        });
    }
}

inline void tryAddToBFSBlendQueue(
        std::queue<BFSBlendElement>& bfsBlendQueue,
        IntVector2 coords,
        Identifiable spreadingZone,
        const std::unordered_map<Identifiable, Matrix<double>, IDHash>& matrices,
        const std::unordered_map<std::pair<Identifiable, Identifiable>, BasicConnection, AsymPairIDHash>& asymEdges
    ){
    if (
        !matrices.at(spreadingZone).isValidPoint(coords)
        || matrices.at(spreadingZone).get(coords) >= 0.001
    ){
        return;
    }
    for (auto& [zoneID, matrix] : matrices){
        if (
            zoneID == spreadingZone
            || matrix.get(coords) <= 0.001
        ){
            continue;
        }
        auto it = asymEdges.find({zoneID, spreadingZone});
        if (it == asymEdges.end()){
            it = asymEdges.find({spreadingZone, zoneID});
        }
        if (it == asymEdges.end()){
            continue;
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
        result[id] = Matrix<double>(grid.getWidth(), grid.getHeight(), 0.0);       
    }
    for (auto it = grid.begin(); it != grid.end(); ++it){
        if (*it == Identifiable::nullID){
            continue;
        }
        result[*it].set(it.getX(), it.getY(), 1.0);
    }
    return result;
}
