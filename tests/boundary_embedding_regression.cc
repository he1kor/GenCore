#include "planar_layout.h"
#include <iostream>
#include <cmath>
#include <stdexcept>

using Edges = std::vector<planar::Edge>;

static void require(bool condition,const char* message) {
    if(!condition)throw std::runtime_error(message);
}

static void checkInteractiveBudget() {
    // Four K4 blocks share one edge. Their order is flexible, so this family
    // exercises the combinatorial fallback rather than the rigid-graph check.
    std::vector<Identifiable> ids;
    std::vector<std::pair<Identifiable,Identifiable>> edges{{0,1}};
    for(int v=0;v<10;++v)ids.emplace_back(v);
    for(int a=2;a<10;a+=2)for(auto [u,v]:std::vector<std::pair<int,int>>{{0,a},{1,a},{0,a+1},{1,a+1},{a,a+1}})
        edges.emplace_back(u,v);
    for(int free=2;free<10;++free) {
        NodeBoundaryMap rules{{0,BoundaryPlacement::Outer},{1,BoundaryPlacement::Outer}};
        for(int v=2;v<10;++v)if(v!=free)rules.emplace(v,BoundaryPlacement::Inner);
        const auto start=std::chrono::steady_clock::now();
        try {
            const auto graph=planar::makeTopology(ids,edges,rules,std::chrono::milliseconds(1));
            require(planar::validBoundaryDrawing(graph->gridDrawing,*graph),"Interactive check returned an invalid drawing");
        }catch(const planar::BoundarySearchLimit&) {
            // Feasibility is inconclusive when time runs out. These graphs
            // are feasible, so ConflictingBoundaryRules must never be thrown.
        }
        require(std::chrono::steady_clock::now()-start<std::chrono::milliseconds(250),
            "Interactive boundary search ignored its time budget");
    }
}

static auto topology(size_t n,const Edges& edges,const std::vector<int>& outer,const std::vector<int>& inner) {
    std::vector<Identifiable> ids;
    std::vector<std::pair<Identifiable,Identifiable>> connections;
    for(size_t v=0;v<n;++v)ids.emplace_back(10+int(v)*3);
    for(auto [a,b]:edges)connections.emplace_back(ids[a],ids[b]);
    NodeBoundaryMap rules;
    for(auto v:outer)rules.emplace(ids[v],BoundaryPlacement::Outer);
    for(auto v:inner)rules.emplace(ids[v],BoundaryPlacement::Inner);
    return planar::makeTopology(std::move(ids),connections,rules);
}

static void check(size_t n,const Edges& edges,const std::vector<int>& outer,const std::vector<int>& inner,bool feasible) {
    std::shared_ptr<const planar::Topology> graph;
    try{graph=topology(n,edges,outer,inner);}
    catch(const planar::ConflictingBoundaryRules&){require(!feasible,"Rejected feasible boundary rules");return;}
    require(feasible,"Accepted incompatible boundary rules");
    require(graph->edges.size()==edges.size(),"Private edges reached real graph");
    for(int seed=0;seed<8;++seed) {
        planar::Layout layout(graph,std::vector<double>(n,1),{},0,0,128,128,seed);
        while(layout.isRunning()) {
            require(planar::validBoundaryDrawing(layout.positions(),*graph),"Relaxation changed boundary status");
            layout.step();
        }
        auto raster=layout.positions();
        for(auto& p:raster){p.x=std::floor(p.x);p.y=std::floor(p.y);}
        require(planar::validBoundaryDrawing(raster,*graph),"Raster boundary mismatch");
        require(planar::validDrawing(raster,graph->edges),"Raster graph crossed");
    }
}

int main() {
    try {
        const Edges triangle{{0,1},{1,2},{2,0}},leaf{{0,1},{1,2},{2,0},{0,3}};
        const Edges k4{{0,1},{0,2},{0,3},{1,2},{1,3},{2,3}};
        check(1,{}, {},{0},false);
        check(2,{{0,1}},{0},{},true);
        check(6,{{0,1},{1,2},{1,3},{3,4},{3,5}}, {},{1},false);
        check(3,triangle, {},{0},false);
        check(4,leaf,{0,1,2},{3},true);
        auto innerLeaf=topology(4,leaf,{0,1,2},{3});
        require(planar::validBoundaryDrawing({{0,0},{10,0},{0,10},{2,2}},*innerLeaf),"Interior leaf was not recognized");
        require(!planar::validBoundaryDrawing({{0,0},{10,0},{0,10},{-2,-2}},*innerLeaf),"Exterior leaf counted as Inner");
        check(4,leaf,{0,1,2,3},{},true);
        check(4,k4,{0,1,2},{3},true);
        check(4,k4,{0,1,2,3},{},false);
        check(4,k4, {},{2,3},false);
        // Degree-two subdivisions retain face requirements on their paths.
        Edges subdivided{{0,4},{4,1},{0,2},{0,3},{1,2},{1,3},{2,3}};
        check(5,subdivided,{0,1,2},{4},false);
        check(5,subdivided, {},{4},true);
        Edges parallel=k4;
        for(size_t v=4;v<7;++v){parallel.emplace_back(0,v);parallel.emplace_back(v,1);}
        check(7,parallel,{0,1,2,4},{5,6},true);
        check(7,parallel,{4,5},{2,3,6},true); // Exterior between parallel paths.
        check(7,parallel,{4,5,6},{},false);
        Edges grid;
        for(size_t y=0;y<3;++y)for(size_t x=0;x<3;++x) {
            if(x<2)grid.emplace_back(y*3+x,y*3+x+1);
            if(y<2)grid.emplace_back(y*3+x,(y+1)*3+x);
        }
        check(9,grid,{0,1,2,3,5,6,7,8},{4},true);
        check(9,grid,{4},{},true);
        check(9,grid,{0,1,2,3,4,5,6,7,8},{},false);
        Edges octahedron;
        for(size_t a=0;a<6;++a)for(size_t b=a+1;b<6;++b)if(a/2!=b/2)octahedron.emplace_back(a,b);
        check(6,octahedron, {},{0},true);
        check(6,octahedron, {},{0,1},false);
        // Components can be deliberately nested, including isolated vertices.
        check(5,{{0,1},{1,2},{2,0},{3,4}},{0,1,2},{3,4},true);
        check(4,triangle,{0,1,2},{3},true);
        check(6,{{0,1},{1,2},{2,0},{3,4},{4,5},{5,3}},{0,1,2},{3,4,5},true);
        // Many leaf corners should not require factorial search.
        auto fan=triangle;
        std::vector<int> outside{0,1,2},inside;
        for(int v=3;v<23;++v){fan.emplace_back(0,v);(v%2?inside:outside).push_back(v);}
        check(23,fan,outside,inside,true);
        // Direct magnet placements may prefer a forbidden face.
        for(bool inner:{false,true}) {
            auto graph=topology(4,leaf,inner?std::vector<int>{0,1,2}:std::vector<int>{0,1,2,3},inner?std::vector<int>{3}:std::vector<int>{});
            PositionMagnetMap magnets{{graph->ids[3],{{.5,.5},100}}};
            planar::Layout layout(graph,std::vector<double>(4,1),{},0,0,128,128,42,magnets);
            while(layout.step())require(planar::validBoundaryDrawing(layout.positions(),*graph),"Magnet broke boundary rule");
        }
        checkInteractiveBudget();
        std::cout<<"Boundary topology, component nesting, relaxation, magnets and raster checks passed\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
