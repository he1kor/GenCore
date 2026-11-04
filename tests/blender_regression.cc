#include "blender.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

struct TestZone : Identifiable { using Identifiable::Identifiable; };
using TestGraph = EdgeGraph<TestZone, BasicSymConnection, BasicAsymConnection>;
using SymEdges = std::unordered_map<std::pair<Identifiable,Identifiable>, BasicSymConnection, PairIDHash>;
using AsymEdges = std::unordered_map<std::pair<Identifiable,Identifiable>, BasicAsymConnection, AsymPairIDHash>;

static void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

static TestGraph graph(const SymEdges& sym, const AsymEdges& asym = {}) {
    std::vector<std::pair<TestZone,std::vector<Identifiable>>> nodes;
    for (int id : {1,2,3}) {
        std::vector<Identifiable> neighbours;
        for (const auto& [pair,edge] : sym) {
            if (pair.first == Identifiable(id)) neighbours.push_back(pair.second);
            if (pair.second == Identifiable(id)) neighbours.push_back(pair.first);
        }
        nodes.push_back({TestZone(id),neighbours});
    }
    return TestGraph(nodes,sym,asym);
}

static Grid<TestZone> stripes() {
    Grid<TestZone> grid(12,6,std::vector<TestZone>{1,2,3});
    for (int y=0;y<6;++y)
        for (int x=0;x<12;++x) grid.setTile(x,y,x<4 ? 1 : x<6 ? 2 : 3);
    return grid;
}

static void normalized(const ZoneMasks& masks, const Grid<TestZone>& grid) {
    for (int y=0;y<grid.getHeight();++y) {
        for (int x=0;x<grid.getWidth();++x) {
            double sum=0;
            for (const auto& [id,mask] : masks.influenceMask) {
                require(mask.get(x,y)>=0.0 && mask.get(x,y)<=1.0,"Influence outside [0,1]");
                sum+=mask.get(x,y);
            }
            const double expected=grid.getTileID(x,y)==Identifiable::nullID ? 0.0 : 1.0;
            require(std::abs(sum-expected)<1e-12,"Influences do not sum to one (or zero on walls)");
        }
    }
}

static void excludesThirdZones(int distance, bool includeThirdEdge) {
    auto grid=stripes();
    SymEdges edges={{{1,2},BasicSymConnection(8,{})}};
    if (includeThirdEdge) edges[{2,3}]=BasicSymConnection(distance,{});
    const auto masks=blendConnections(grid,graph(edges));
    require(masks.influenceMask.at(1).get(4,3)>0.0,"Configured blend disappeared");
    require(masks.influenceMask.at(2).get(3,3)>0.0,"Reverse blend disappeared");
    for (int y=0;y<6;++y) {
        for (int x=6;x<12;++x) {
            require(masks.influenceMask.at(1).get(x,y)==0.0,"Zone 1 leaked through zone 2 into zone 3");
            if (distance<=0) require(masks.influenceMask.at(2).get(x,y)==0.0,"Disabled connection still blends");
        }
    }
    normalized(masks,grid);
}

static void independentDistances() {
    Grid<TestZone> grid(18,8,std::vector<TestZone>{1,2,3});
    for(int y=0;y<8;++y)
        for(int x=0;x<18;++x) grid.setTile(x,y,x<6 ? 2 : x<12 ? 1 : 3);
    const auto masks=blendConnections(grid,graph({
        {{1,2},BasicSymConnection(8,{})},{{1,3},BasicSymConnection(3,{})}
    }));
    require(masks.influenceMask.at(1).get(0,4)>0.0,"Long connection was suppressed by another pair");
    require(std::abs(masks.influenceMask.at(1).get(12,4)-3.0/7.0)<1e-12,"First blend layer changed its falloff");
    require(std::abs(masks.influenceMask.at(1).get(13,4)-1.0/3.0)<1e-12,"Second blend layer changed its falloff");
    require(masks.influenceMask.at(1).get(14,4)==0.0,"Long connection overrode the short connection distance");
    normalized(masks,grid);
}

static void preservesIntakeOwnership() {
    auto grid=stripes();
    const auto masks=blendConnections(grid,graph(
        {{{1,2},BasicSymConnection(0,{})},{{2,3},BasicSymConnection(4,{})}},
        {{{1,2},BasicAsymConnection(1,0,0.0)}}
    ));
    require(masks.influenceMask.at(2).get(3,3)>0.0,"Intake into zone 1 was lost");
    require(masks.influenceMask.at(1).get(3,3)==0.0,"Blending restored pre-intake ownership");
    require(masks.influenceMask.at(3).get(3,3)>0.0,"Blending ignored post-intake ownership");
    require(masks.influenceMask.at(3).get(2,3)==0.0,"Blend escaped the post-intake receiving zone");
    normalized(masks,grid);
}

static void wallsBlockPropagation() {
    auto grid=stripes();
    for(int y=0;y<6;++y) grid.setTile(4,y,Identifiable::nullID);
    const auto masks=blendConnections(grid,graph({{{1,2},BasicSymConnection(8,{})}}));
    require(masks.influenceMask.at(1).get(5,3)==0.0,"Blend crossed a wall");
    normalized(masks,grid);
}

int main() {
    try {
        excludesThirdZones(0,false);
        excludesThirdZones(0,true);
        excludesThirdZones(-2,true);
        excludesThirdZones(3,true);
        independentDistances();
        preservesIntakeOwnership();
        wallsBlockPropagation();
        std::cout << "All 7 blending regression cases passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
