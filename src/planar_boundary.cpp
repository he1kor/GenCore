#include "planar_layout.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/boyer_myrvold_planar_test.hpp>
#include <boost/graph/chrobak_payne_drawing.hpp>
#include <boost/graph/planar_canonical_ordering.hpp>
#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <sstream>

namespace planar {
namespace {
using Rotation = std::vector<std::vector<size_t>>;
using Walk = std::vector<size_t>;
using Graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
    boost::no_property, boost::property<boost::edge_index_t, size_t>>;
using Descriptor = boost::graph_traits<Graph>::edge_descriptor;
using Embedding = std::vector<std::vector<Descriptor>>;

struct SearchBudget {
    using Clock=std::chrono::steady_clock;
    size_t states=0;
    Clock::time_point deadline;
    explicit SearchBudget(std::chrono::milliseconds duration)
        :deadline(duration.count()>0?Clock::now()+duration:Clock::time_point::max()){}
    void checkTime() const {
        if(deadline!=Clock::time_point::max()&&Clock::now()>=deadline)
            throw BoundarySearchLimit("Boundary placement check reached its interactive time limit; feasibility is inconclusive. The rules have not been weakened. Use for generation runs the full check.");
    }
    void step() {
        checkTime();
        if(++states>200000)throw BoundarySearchLimit("Boundary placement search limit reached; no compatible embedding was certified. This does not prove the rules are impossible. Simplify the graph or relax some boundary rules.");
    }
};

std::vector<Walk> faceWalks(const Rotation& rotation) {
    std::set<Edge> visited;
    std::vector<Walk> result;
    for (size_t u = 0; u < rotation.size(); ++u) for (size_t v : rotation[u]) {
        if (visited.contains({u,v})) continue;
        Walk face;
        size_t a = u, b = v;
        do {
            if (!visited.emplace(a,b).second) throw std::logic_error("Invalid boundary rotation system");
            face.push_back(a);
            const auto& around = rotation[b];
            const size_t at = std::find(around.begin(),around.end(),a)-around.begin();
            const size_t next = around[(at+around.size()-1)%around.size()];
            a=b; b=next;
        } while (a!=u || b!=v);
        result.push_back(std::move(face));
    }
    return result;
}

bool embed(Graph& graph, Embedding& embedding) {
    size_t index=0;
    for (auto [it,end]=boost::edges(graph);it!=end;++it) boost::put(boost::edge_index,graph,*it,index++);
    embedding.assign(boost::num_vertices(graph),{});
    return boost::boyer_myrvold_planarity_test(boost::boyer_myrvold_params::graph=graph,
        boost::boyer_myrvold_params::embedding=embedding.data());
}

Rotation rotationOf(const Graph& graph,const Embedding& embedding,size_t count) {
    Rotation result(count);
    for(size_t v=0;v<count;++v) for(auto e:embedding[v]) {
        const size_t a=boost::source(e,graph),b=boost::target(e,graph),u=a==v?b:a;
        if(u<count)result[v].push_back(u);
    }
    return result;
}

bool matches(const Walk& face,const std::vector<BoundaryPlacement>& rules) {
    std::vector<bool> incident(rules.size());
    for(auto v:face)incident[v]=true;
    for(size_t v=0;v<rules.size();++v)
        if((rules[v]==BoundaryPlacement::Outer&&!incident[v])
            ||(rules[v]==BoundaryPlacement::Inner&&incident[v]))return false;
    return true;
}

struct Choice { Rotation rotation; Walk outer; };

std::optional<Choice> selectFace(const Rotation& rotation,const std::vector<BoundaryPlacement>& rules) {
    auto faces=faceWalks(rotation);
    size_t twiceEdges=0;
    for(const auto& row:rotation)twiceEdges+=row.size();
    if(faces.size()!=twiceEdges/2+2-rotation.size())return {};
    std::stable_sort(faces.begin(),faces.end(),[](const auto& a,const auto& b){return a.size()>b.size();});
    for(const auto& face:faces)if(matches(face,rules))return Choice{rotation,face};
    return {};
}

// Inner leaves can choose another corner of their parent's rotation. Doing
// this directly avoids factorially permuting many interchangeable leaf edges.
std::optional<Choice> placeInnerLeaves(const Rotation& rotation,const std::vector<BoundaryPlacement>& rules) {
    auto reduced=rotation;
    auto reducedRules=rules;
    std::vector<size_t> leaves;
    for(size_t v=0;v<rotation.size();++v)if(rotation[v].size()==1&&rules[v]==BoundaryPlacement::Inner) {
        leaves.push_back(v);
        std::erase(reduced[rotation[v][0]],v);
        reduced[v].clear(); reducedRules[v]=BoundaryPlacement::Any;
    }
    if(leaves.empty())return {};
    const auto faces=faceWalks(reduced);
    for(size_t exterior=0;exterior<faces.size();++exterior) {
        if(!matches(faces[exterior],reducedRules))continue;
        auto candidate=reduced;
        bool fitting=true;
        for(auto leaf:leaves) {
            const auto parent=rotation[leaf][0];
            bool placed=false;
            for(size_t f=0;f<faces.size()&&!placed;++f)if(f!=exterior) {
                const auto& face=faces[f];
                const auto at=std::find(face.begin(),face.end(),parent);
                if(at==face.end())continue;
                const size_t incoming=face[(at-face.begin()+face.size()-1)%face.size()];
                auto& around=candidate[parent];
                around.insert(std::find(around.begin(),around.end(),incoming),leaf);
                candidate[leaf]={parent}; placed=true;
            }
            if(!placed){fitting=false;break;}
        }
        if(fitting)if(auto choice=selectFace(candidate,rules))return choice;
    }
    return {};
}

bool allowedCycle(const Rotation& adjacent,const std::vector<BoundaryPlacement>& rules) {
    std::vector<size_t> parent(adjacent.size());
    std::iota(parent.begin(),parent.end(),0);
    std::function<size_t(size_t)> root=[&](size_t v){return parent[v]==v?v:parent[v]=root(parent[v]);};
    for(size_t v=0;v<adjacent.size();++v)if(rules[v]!=BoundaryPlacement::Inner)
        for(auto u:adjacent[v])if(v<u&&rules[u]!=BoundaryPlacement::Inner) {
            const auto a=root(v),b=root(u);
            if(a==b)return true;
            parent[a]=b;
        }
    return false;
}

bool threeConnected(const Rotation& adjacent,const SearchBudget& budget) {
    const size_t n=adjacent.size();
    if(n<4||std::any_of(adjacent.begin(),adjacent.end(),[](const auto& row){return row.size()<3;}))return false;
    for(size_t a=0;a<n;++a)for(size_t b=a+1;b<n;++b) {
        budget.checkTime();
        std::vector<bool> seen(n); seen[a]=seen[b]=true;
        size_t start=0;while(seen[start])++start;
        Walk todo{start};seen[start]=true;
        for(size_t i=0;i<todo.size();++i)for(auto u:adjacent[todo[i]])if(!seen[u]){seen[u]=true;todo.push_back(u);}
        if(todo.size()!=n-2)return false;
    }
    return true;
}

// Subdivision vertices make a rigid planar graph appear only 2-connected.
// Suppress degree-two paths and retain their face requirements on the resulting
// edges. Parallel paths can surround digon faces, so check those as well as
// the unique simple kernel embedding before certifying a contradiction.
bool subdivisionConflict(const Rotation& adjacent,const std::vector<BoundaryPlacement>& rules,const SearchBudget& budget) {
    const size_t n=adjacent.size();
    Walk vertices,index(n,n);
    for(size_t v=0;v<n;++v)if(adjacent[v].size()!=2) {
        if(adjacent[v].size()<3)return false;
        index[v]=vertices.size();vertices.push_back(v);
    }
    if(vertices.size()==n||vertices.size()<4)return false;
    std::set<Edge> visited;
    std::map<Edge,std::vector<BoundaryPlacement>> paths;
    for(auto start:vertices)for(auto next:adjacent[start]) {
        if(visited.contains({std::min(start,next),std::max(start,next)}))continue;
        size_t previous=start,current=next;
        bool outer=false,inner=false;
        visited.emplace(std::min(previous,current),std::max(previous,current));
        while(adjacent[current].size()==2) {
            outer|=rules[current]==BoundaryPlacement::Outer;
            inner|=rules[current]==BoundaryPlacement::Inner;
            const auto& around=adjacent[current];
            const auto following=around[0]==previous?around[1]:around[0];
            previous=current;current=following;
            visited.emplace(std::min(previous,current),std::max(previous,current));
        }
        if(outer&&inner)return true; // Every internal vertex sees the same two faces.
        if(current==start)return false; // Attached cycles require separate handling.
        const auto a=index[start],b=index[current];
        paths[{std::min(a,b),std::max(a,b)}].push_back(outer?BoundaryPlacement::Outer:inner?BoundaryPlacement::Inner:BoundaryPlacement::Any);
    }
    Rotation kernel(vertices.size());
    Graph graph(vertices.size());
    for(const auto& [edge,bundle]:paths) {
        kernel[edge.first].push_back(edge.second);kernel[edge.second].push_back(edge.first);
        boost::add_edge(edge.first,edge.second,graph);
    }
    if(!threeConnected(kernel,budget))return false;
    Embedding embedding;
    if(!embed(graph,embedding))throw std::logic_error("Suppressing degree-two paths lost planarity");
    std::vector<BoundaryPlacement> kernelRules;
    for(auto v:vertices)kernelRules.push_back(rules[v]);
    for(const auto& face:faceWalks(rotationOf(graph,embedding,vertices.size()))) {
        if(!matches(face,kernelRules))continue;
        std::set<Edge> exposed;
        for(size_t i=0;i<face.size();++i) {
            const auto a=face[i],b=face[(i+1)%face.size()];
            exposed.emplace(std::min(a,b),std::max(a,b));
        }
        bool compatible=true;
        for(const auto& [edge,bundle]:paths) {
            const auto outer=std::count(bundle.begin(),bundle.end(),BoundaryPlacement::Outer);
            const auto available=std::count_if(bundle.begin(),bundle.end(),[](auto rule){return rule!=BoundaryPlacement::Inner;});
            if(exposed.contains(edge)?outer>1||available==0:outer>0){compatible=false;break;}
        }
        if(compatible)return false;
    }
    for(const auto& [edge,bundle]:paths) {
        if(bundle.size()<2||!matches({edge.first,edge.second},kernelRules))continue;
        const auto outer=std::count(bundle.begin(),bundle.end(),BoundaryPlacement::Outer);
        const auto available=std::count_if(bundle.begin(),bundle.end(),[](auto rule){return rule!=BoundaryPlacement::Inner;});
        if(outer>2||available<2)continue;
        bool compatible=true;
        for(const auto& [other,otherBundle]:paths)if(other!=edge
            &&std::find(otherBundle.begin(),otherBundle.end(),BoundaryPlacement::Outer)!=otherBundle.end()){compatible=false;break;}
        if(compatible)return false;
    }
    return true;
}

std::optional<Choice> chooseComponent(const Rotation& adjacent,const std::vector<BoundaryPlacement>& rules,SearchBudget& budget) {
    const size_t n=adjacent.size();
    if(n==1)return rules[0]==BoundaryPlacement::Inner?std::optional<Choice>{}:Choice{adjacent,{0}};
    const bool inner=std::find(rules.begin(),rules.end(),BoundaryPlacement::Inner)!=rules.end();
    if(inner&&!allowedCycle(adjacent,rules))return {};
    Graph graph(n);
    for(size_t v=0;v<n;++v)for(auto u:adjacent[v])if(v<u)boost::add_edge(v,u,graph);
    Embedding embedding;
    if(!embed(graph,embedding))throw NonPlanarGraph("a planar zone graph");
    auto rotation=rotationOf(graph,embedding,n);
    const auto consider=[&](const Rotation& candidate)->std::optional<Choice>{
        if(auto choice=selectFace(candidate,rules))return choice;
        return placeInnerLeaves(candidate,rules);
    };
    if(auto choice=consider(rotation))return choice;
    // A 3-connected planar graph has a unique spherical embedding, up to
    // reflection. Its complete face list is an exact infeasibility certificate.
    if(threeConnected(adjacent,budget)||subdivisionConflict(adjacent,rules,budget))return {};

    bool outer=false;
    for(auto rule:rules)outer|=rule==BoundaryPlacement::Outer;
    if(outer) {
        boost::add_vertex(graph);
        for(size_t v=0;v<n;++v)if(rules[v]==BoundaryPlacement::Outer)boost::add_edge(v,n,graph);
        if(!embed(graph,embedding))return {};
        if(auto choice=consider(rotationOf(graph,embedding,n)))return choice;
    }
    // Exact bounded fallback over cyclic orders. No random rejection can be
    // mistaken for a contradiction. Most templates use one of the fast paths.
    rotation=adjacent;
    Walk branch;
    for(size_t v=0;v<n;++v) {
        std::sort(rotation[v].begin(),rotation[v].end());
        if(rotation[v].size()>2)branch.push_back(v);
    }
    std::stable_sort(branch.begin(),branch.end(),[&](auto a,auto b){return rotation[a].size()<rotation[b].size();});
    std::optional<Choice> result;
    std::function<bool(size_t)> search=[&](size_t depth) {
        budget.step();
        if(depth==branch.size()) {result=selectFace(rotation,rules);return bool(result);}
        auto& around=rotation[branch[depth]];
        do {
            if(depth==0&&around[1]>around.back())continue;
            if(search(depth+1))return true;
        }while(std::next_permutation(around.begin()+1,around.end()));
        return false;
    };
    search(0);
    return result;
}

struct Component {
    Walk vertices;
    Rotation adjacent;
    std::vector<BoundaryPlacement> rules;
    std::optional<Choice> choice;
    bool nested=false;
    bool cyclic=false;
};

void insertBridge(Rotation& rotation,const Walk& first,const Walk& second) {
    const auto a=first.front(),b=second.front();
    auto insert=[&](size_t v,size_t u,const Walk& face) {
        auto& around=rotation[v];
        const auto at=around.empty()?around.end():std::find(around.begin(),around.end(),face.back());
        around.insert(at,u);
    };
    insert(a,b,first);insert(b,a,second);
}

// Reindex a private exterior cap first, so canonical ordering starts on a
// cap/frame edge. Every face incident to the cap lies outside the frame ring.
void integerFallback(Topology& topology) {
    const size_t count=topology.scaffoldSize;
    Graph graph(count+1);
    std::vector<size_t> mapped(count);
    size_t next=2;
    for(size_t v=0;v<count;++v)mapped[v]=v==topology.boundary.front()?1:next++;
    boost::add_edge(0,1,graph);
    for(auto v:topology.boundary)if(mapped[v]!=1)boost::add_edge(0,mapped[v],graph);
    for(auto [a,b]:topology.scaffoldEdges)boost::add_edge(mapped[a],mapped[b],graph);
    Embedding embedding;
    if(!embed(graph,embedding)||boost::num_edges(graph)!=3*(count+1)-6)
        throw std::logic_error("Boundary scaffold is not a triangulated disk");
    std::vector<size_t> ordering;
    auto embeddingMap=boost::make_iterator_property_map(embedding.begin(),boost::get(boost::vertex_index,graph));
    boost::planar_canonical_ordering(graph,embeddingMap,std::back_inserter(ordering));
    struct Point {size_t x=0,y=0;};
    std::vector<Point> grid(count+1);
    auto drawing=boost::make_iterator_property_map(grid.begin(),boost::get(boost::vertex_index,graph));
    boost::chrobak_payne_straight_line_drawing(graph,embeddingMap,ordering.begin(),ordering.end(),drawing);
    for(size_t v=0;v<topology.ids.size();++v)topology.gridDrawing.emplace_back(grid[mapped[v]].x,grid[mapped[v]].y);
    if(!validDrawing(topology.gridDrawing,topology.edges)||!validBoundaryDrawing(topology.gridDrawing,topology))
        throw std::logic_error("Integer fallback violated boundary placement");
}
} // namespace

void constrainBoundary(Topology& topology,std::chrono::milliseconds searchTimeLimit) {
    SearchBudget budget(searchTimeLimit);
    const size_t n=topology.ids.size();
    if(n<3) {
        for(size_t v=0;v<n;++v)if(topology.nodeBoundary[v]==BoundaryPlacement::Inner)
            throw ConflictingBoundaryRules("Inner zones need enclosure by real connection cycles; this graph has no bounded face.");
        topology.scaffoldEdges=topology.edges;
        topology.rotation.resize(n);
        for(auto [a,b]:topology.edges){topology.rotation[a].push_back(b);topology.rotation[b].push_back(a);}
        for(size_t v=0;v<n;++v){topology.gridDrawing.emplace_back(v,0);topology.outerFace.push_back(v);}
        return;
    }
    Rotation adjacent(n);
    for(auto [a,b]:topology.edges){adjacent[a].push_back(b);adjacent[b].push_back(a);}
    Walk component(n,n);
    std::vector<Component> parts;
    for(size_t v=0;v<n;++v)if(component[v]==n) {
        const auto index=parts.size();
        Component part;part.vertices={v};component[v]=index;
        for(size_t i=0;i<part.vertices.size();++i)for(auto u:adjacent[part.vertices[i]])if(component[u]==n){component[u]=index;part.vertices.push_back(u);}
        std::sort(part.vertices.begin(),part.vertices.end());
        Walk local(n,n);
        for(size_t i=0;i<part.vertices.size();++i)local[part.vertices[i]]=i;
        size_t twiceEdges=0;
        for(auto u:part.vertices) {
            Walk row;for(auto w:adjacent[u])row.push_back(local[w]);
            twiceEdges+=row.size();part.adjacent.push_back(std::move(row));
            part.rules.push_back(topology.nodeBoundary[u]);
        }
        part.cyclic=twiceEdges/2>=part.vertices.size();
        const bool outer=std::find(part.rules.begin(),part.rules.end(),BoundaryPlacement::Outer)!=part.rules.end();
        const bool inner=std::find(part.rules.begin(),part.rules.end(),BoundaryPlacement::Inner)!=part.rules.end();
        part.nested=inner&&!outer;
        parts.push_back(std::move(part));
    }
    size_t host=parts.size();
    const auto conflict=[&]() -> void {
        std::ostringstream message;
        message<<"Boundary placement rules cannot share a valid outer face. Zones";
        for(size_t v=0;v<n;++v)if(topology.nodeBoundary[v]!=BoundaryPlacement::Any)
            message<<' '<<topology.ids[v].getID()<<'='<<(topology.nodeBoundary[v]==BoundaryPlacement::Outer?"Outer":"Inner");
        message<<". Change boundary rules or real connections; inner zones require real enclosing cycles.";
        throw ConflictingBoundaryRules(message.str());
    };
    for(size_t i=0;i<parts.size();++i)if(!parts[i].nested) {
        parts[i].choice=chooseComponent(parts[i].adjacent,parts[i].rules,budget);
        if(!parts[i].choice)conflict();
        if(parts[i].cyclic)host=i;
    }
    const bool nesting=std::any_of(parts.begin(),parts.end(),[](const auto& part){return part.nested;});
    if(nesting&&host==parts.size()) {
        bool limited=false;
        for(size_t i=0;i<parts.size();++i)if(parts[i].nested&&parts[i].cyclic) {
            try {parts[i].choice=chooseComponent(parts[i].adjacent,parts[i].rules,budget);}
            catch(const BoundarySearchLimit&){limited=true;continue;}
            if(parts[i].choice){parts[i].nested=false;host=i;break;}
        }
        if(host==parts.size()) {
            if(limited)throw BoundarySearchLimit("Boundary placement search limit reached while selecting an enclosing component; feasibility is inconclusive.");
            conflict();
        }
    }
    for(auto& part:parts)if(part.nested) {
        const std::vector<BoundaryPlacement> unrestricted(part.vertices.size(),BoundaryPlacement::Any);
        part.choice=chooseComponent(part.adjacent,unrestricted,budget);
        if(!part.choice)throw std::logic_error("Could not embed a nested planar component");
    }
    Rotation rotation(n);
    std::vector<Walk> exteriors;
    for(const auto& part:parts) {
        for(size_t v=0;v<part.vertices.size();++v)for(auto u:part.choice->rotation[v])rotation[part.vertices[v]].push_back(part.vertices[u]);
        Walk outer;for(auto v:part.choice->outer)outer.push_back(part.vertices[v]);
        exteriors.push_back(std::move(outer));
    }
    size_t first=0;while(parts[first].nested)++first;
    for(size_t i=0;i<parts.size();++i)if(i!=first&&!parts[i].nested)insertBridge(rotation,exteriors[first],exteriors[i]);
    if(nesting) {
        const auto localFaces=faceWalks(parts[host].choice->rotation);
        Walk pocket;
        for(const auto& face:localFaces)if(face!=parts[host].choice->outer) {
            for(auto v:face)pocket.push_back(parts[host].vertices[v]);
            break;
        }
        if(pocket.empty())throw std::logic_error("Enclosing component has no bounded face");
        for(size_t i=0;i<parts.size();++i)if(parts[i].nested)insertBridge(rotation,pocket,exteriors[i]);
    }
    auto selected=selectFace(rotation,topology.nodeBoundary);
    if(!selected)throw std::logic_error("Component assembly lost boundary constraints");
    topology.rotation=rotation;
    std::set<Edge> real(topology.edges.begin(),topology.edges.end()),scaffold;
    const auto add=[&](size_t a,size_t b){if(a!=b)scaffold.emplace(std::min(a,b),std::max(a,b));};
    for(size_t v=0;v<n;++v)for(auto u:rotation[v])add(v,u);
    for(size_t v=0;v<n;++v)std::erase_if(topology.rotation[v],[&](size_t u){return !real.contains({std::min(v,u),std::max(v,u)});});
    topology.outerFace=selected->outer;
    std::reverse(topology.outerFace.begin(),topology.outerFace.end());
    topology.scaffoldSize=n;
    for(const auto& face:faceWalks(rotation)) {
        const bool outer=face==selected->outer;
        const bool simple=std::set<size_t>(face.begin(),face.end()).size()==face.size();
        if(!outer&&simple) {
            if(face.size()>3) {
                const auto center=topology.scaffoldSize++;
                for(auto v:face)add(center,v);
            }
            continue;
        }
        const auto start=topology.scaffoldSize;
        topology.scaffoldSize+=face.size();
        for(size_t i=0;i<face.size();++i) {
            const auto j=(i+1)%face.size();
            add(start+i,start+j);add(start+i,face[i]);add(start+i,face[j]);
            if(outer)topology.boundary.push_back(start+i);
        }
        if(!outer) {
            const auto center=topology.scaffoldSize++;
            for(size_t i=0;i<face.size();++i)add(center,start+i);
        }
    }
    topology.scaffoldEdges.assign(scaffold.begin(),scaffold.end());
    integerFallback(topology);
}

bool validBoundaryDrawing(const std::vector<DoubleVector2>& positions,const Topology& topology) {
    if(topology.nodeBoundary.empty())return true;
    const size_t n=topology.ids.size();
    if(positions.size()!=n||topology.nodeBoundary.size()!=n)return false;
    Rotation rotation(n);
    for(auto p:positions)if(!std::isfinite(p.x)||!std::isfinite(p.y))return false;
    for(auto [a,b]:topology.edges){rotation[a].push_back(b);rotation[b].push_back(a);}
    for(size_t v=0;v<n;++v)std::sort(rotation[v].begin(),rotation[v].end(),[&](auto a,auto b){
        return std::atan2(positions[a].y-positions[v].y,positions[a].x-positions[v].x)
            <std::atan2(positions[b].y-positions[v].y,positions[b].x-positions[v].x);
    });
    Walk component(n,n),representative;
    for(size_t v=0;v<n;++v)if(component[v]==n) {
        const auto label=representative.size();representative.push_back(v);
        Walk todo{v};component[v]=label;
        for(size_t i=0;i<todo.size();++i)for(auto u:rotation[todo[i]])if(component[u]==n){component[u]=label;todo.push_back(u);}
    }
    std::vector<Walk> exterior(representative.size());
    std::vector<long double> least(representative.size(),0);
    for(const auto& face:faceWalks(rotation)) {
        long double area=0;
        for(size_t i=0;i<face.size();++i) {
            const auto a=positions[face[i]],b=positions[face[(i+1)%face.size()]];
            area+=static_cast<long double>(a.x)*b.y-static_cast<long double>(a.y)*b.x;
        }
        const auto c=component[face.front()];
        if(exterior[c].empty()||area<least[c]){least[c]=area;exterior[c]=face;}
    }
    for(size_t c=0;c<exterior.size();++c)if(exterior[c].empty())exterior[c]={representative[c]};
    const auto inside=[&](DoubleVector2 p,const Walk& polygon) {
        int winding=0;
        for(size_t i=0;i<polygon.size();++i) {
            const auto a=positions[polygon[i]],b=positions[polygon[(i+1)%polygon.size()]];
            if(a.y<=p.y&&b.y>p.y&&orientation(a,b,p)>0)++winding;
            if(a.y>p.y&&b.y<=p.y&&orientation(a,b,p)<0)--winding;
        }
        return winding!=0;
    };
    std::vector<bool> outer(n);
    for(size_t c=0;c<exterior.size();++c) {
        bool nested=false;
        for(size_t d=0;d<exterior.size()&&!nested;++d)if(c!=d)nested=inside(positions[representative[c]],exterior[d]);
        if(!nested)for(auto v:exterior[c])outer[v]=true;
    }
    for(size_t v=0;v<n;++v)
        if((topology.nodeBoundary[v]==BoundaryPlacement::Outer&&!outer[v])
            ||(topology.nodeBoundary[v]==BoundaryPlacement::Inner&&outer[v]))return false;
    return true;
}
} // namespace planar
