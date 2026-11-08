#include "planar_symmetry.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/boyer_myrvold_planar_test.hpp>
#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <numeric>
#include <optional>
#include <set>

namespace planar {
namespace {
using TestGraph = boost::adjacency_list<boost::setS, boost::vecS, boost::undirectedS>;
using Points = std::vector<DoubleVector2>;

std::vector<double> solve(std::vector<std::vector<double>> matrix) {
    const size_t n = matrix.size();
    for (size_t k = 0; k < n; ++k) {
        size_t pivot = k;
        for (size_t i = k + 1; i < n; ++i)
            if (std::abs(matrix[i][k]) > std::abs(matrix[pivot][k])) pivot = i;
        if (std::abs(matrix[pivot][k]) < 1e-12) return {};
        std::swap(matrix[k], matrix[pivot]);
        for (size_t i = k + 1; i < n; ++i) {
            const double factor = matrix[i][k] / matrix[k][k];
            for (size_t j = k + 1; j <= n; ++j) matrix[i][j] -= factor * matrix[k][j];
        }
    }
    std::vector<double> result(n);
    for (size_t i = n; i-- > 0;) {
        double value = matrix[i][n];
        for (size_t j = i + 1; j < n; ++j) value -= matrix[i][j] * result[j];
        result[i] = value / matrix[i][i];
    }
    return result;
}

class Search {
    const Topology& topology;
    const PositionMagnetMap& magnets;
    double width, height;
    bool gridAligned;
    const std::function<bool(const Reflection&)>& accept;
    size_t n, states = 0, tests = 0;
    std::vector<std::vector<bool>> adjacent;
    std::vector<std::vector<size_t>> neighbours;
    std::vector<size_t> partner;
    std::vector<int> colors;
    bool structuralReflection = false;
    bool settingsRejected = false;
    bool matchingSettings = false;
    bool preferExistingFaces = true;
    std::shared_ptr<Reflection> result;

    // Keep a reflective rotation system and outer face from the ordinary
    // initializer. Face centers produce a symmetric triangulated disk without
    // choosing a different diagonal or folding a degree-two node onto an edge.
    Points existingFacesDrawing() const {
        const auto& rotation = topology.rotation;
        const auto& boundary = topology.outerFace;
        const size_t m = boundary.size();
        if (rotation.size() != n || m < 3 || std::set<size_t>(boundary.begin(),boundary.end()).size() != m) return {};
        auto cyclicEqual = [](const auto& a, const auto& b) {
            if (a.size() != b.size()) return false;
            if (a.empty()) return true;
            for (size_t offset = 0; offset < a.size(); ++offset) {
                bool equal = true;
                for (size_t i = 0; i < a.size(); ++i) if (a[i] != b[(i+offset)%b.size()]) { equal=false;break; }
                if (equal) return true;
            }
            return false;
        };
        for (size_t v = 0; v < n; ++v) {
            std::vector<size_t> reflected;
            for (auto it=rotation[v].rbegin();it!=rotation[v].rend();++it) reflected.push_back(partner[*it]);
            if (!cyclicEqual(reflected,rotation[partner[v]])) return {};
        }
        const auto start = std::find(boundary.begin(),boundary.end(),partner[boundary.front()]);
        if (start == boundary.end()) return {};
        const size_t shift = start-boundary.begin();
        for (size_t i=0;i<m;++i) {
            if (partner[boundary[i]] != boundary[(shift+m-i)%m]
                || !adjacent[boundary[i]][boundary[(i+1)%m]]) return {};
        }
        std::set<Edge> visited;
        std::vector<std::vector<size_t>> boundedFaces;
        bool foundOuter = false;
        auto reversedBoundary = boundary;
        std::reverse(reversedBoundary.begin(),reversedBoundary.end());
        for (size_t a=0;a<n;++a) for (auto b:rotation[a]) {
            if (visited.contains({a,b})) continue;
            std::vector<size_t> face;
            size_t u=a,v=b;
            do {
                if (!visited.emplace(u,v).second) return {};
                face.push_back(u);
                const auto& row=rotation[v];
                const auto at=std::find(row.begin(),row.end(),u)-row.begin();
                if (at == row.size()) return {};
                const auto next=row[(at+row.size()-1)%row.size()];u=v;v=next;
            } while (u!=a || v!=b);
            if (cyclicEqual(face,reversedBoundary)) { foundOuter=true;continue; }
            if (face.size()<3 || std::set<size_t>(face.begin(),face.end()).size()!=face.size()) return {};
            boundedFaces.push_back(std::move(face));
        }
        if (!foundOuter || visited.size()!=2*topology.edges.size()) return {};
        const size_t count=n+boundedFaces.size();
        std::vector<std::vector<size_t>> graph=neighbours;
        graph.resize(count);
        for (size_t f=0;f<boundedFaces.size();++f) for (auto v:boundedFaces[f]) {
            graph[n+f].push_back(v);graph[v].push_back(n+f);
        }
        Points positions(count);
        std::vector<bool> fixed(count);
        // Reflection of the outer cycle sends i to shift-i. This phase makes
        // that permutation the geometric reflection x -> -x.
        const double pi=std::acos(-1.0),phase=pi/2-pi*shift/m;
        for (size_t i=0;i<m;++i) {
            positions[boundary[i]]={std::cos(phase+2*pi*i/m),std::sin(phase+2*pi*i/m)};
            fixed[boundary[i]]=true;
        }
        std::vector<size_t> free,index(count,count);
        for (size_t v=0;v<count;++v) if (!fixed[v]) { index[v]=free.size();free.push_back(v); }
        for (bool y:{false,true}) {
            std::vector<std::vector<double>> matrix(free.size(),std::vector<double>(free.size()+1));
            for (size_t i=0;i<free.size();++i) for (auto v:graph[free[i]]) {
                matrix[i][i]+=1;
                if (fixed[v]) matrix[i].back()+=y?positions[v].y:positions[v].x;
                else matrix[i][index[v]]-=1;
            }
            const auto solution=solve(std::move(matrix));
            if (solution.size()!=free.size()) return {};
            for (size_t i=0;i<free.size();++i) (y?positions[free[i]].y:positions[free[i]].x)=solution[i];
        }
        positions.resize(n);
        for (size_t i=0;i<n;++i) if (i<=partner[i]) {
            const size_t j=partner[i];
            const DoubleVector2 p{(positions[i].x-positions[j].x)/2,(positions[i].y+positions[j].y)/2};
            positions[i]=p;positions[j]={-p.x,p.y};
        }
        if (!validDrawing(positions,topology.edges)) return {};
        return positions;
    }

    bool planar(const TestGraph& graph) {
        if (++tests > 12000) throw NoReflection("Symmetry search limit reached; no compatible planar reflection was certified. Choose No symmetry or simplify the graph.");
        return boost::boyer_myrvold_planarity_test(graph);
    }

    bool compatibleAxis(Reflection& reflection) const {
        std::vector<std::optional<DoubleVector2>> targets(n);
        for (size_t i = 0; i < n; ++i) if (const auto it = magnets.find(topology.ids[i]); it != magnets.end()) {
            const auto p = it->second.position;
            if (p.x >= 0 && p.x <= 1 && p.y >= 0 && p.y <= 1) {
                const double x = width >= 2 ? std::clamp(p.x*width,.5,width-.5) : p.x*width;
                const double y = height >= 2 ? std::clamp(p.y*height,.5,height-.5) : p.y*height;
                targets[i] = DoubleVector2{x-width/2,y-height/2};
            }
        }
        auto impose = [&](DoubleVector2 normal) {
            const double length = std::hypot(normal.x, normal.y);
            if (length < 1e-8 * std::max(width, height)) return true;
            normal.x /= length; normal.y /= length;
            if (reflection.constrainedAxis)
                return std::abs(normal.x * reflection.normal.y - normal.y * reflection.normal.x) < 1e-6;
            reflection.normal = normal;
            reflection.constrainedAxis = true;
            return true;
        };
        for (size_t i = 0; i < n; ++i) {
            const size_t j = partner[i];
            if (!targets[i] || i > j) continue;
            const auto p = *targets[i];
            if (i == j) {
                if (!impose({-p.y, p.x})) return false;
            } else if (targets[j]) {
                const auto q = *targets[j];
                const DoubleVector2 delta{p.x - q.x, p.y - q.y};
                if (std::hypot(delta.x, delta.y) < 1e-8 * std::max(width, height)) {
                    if (!impose({-p.y, p.x})) return false;
                } else if (!impose(delta)) return false;
            }
        }
        const auto normal = reflection.normal;
        for (size_t i = 0; i < n; ++i) if (targets[i] && targets[partner[i]]) {
            const auto p = *targets[i], q = *targets[partner[i]];
            const double dot = p.x * normal.x + p.y * normal.y;
            if (std::hypot(p.x - 2 * dot * normal.x - q.x, p.y - 2 * dot * normal.y - q.y)
                > 1e-5 * std::max(width, height)) return false;
        }
        if (gridAligned && reflection.constrainedAxis) {
            const auto axis=tileAxisForNormal(normal,width,height);
            if (!axis) return false;
            reflection.normal=tileAxisNormal(*axis);
        }
        return true;
    }

    Points halfDrawing(const std::vector<size_t>& halfIndex, size_t halfSize,
        const std::vector<Edge>& halfEdges, const std::vector<std::pair<size_t,size_t>>& midpoints,
        const std::vector<size_t>& order) {
        const size_t top = halfSize, bottom = top + 1, leftTop = top + 2, leftBottom = top + 3;
        const size_t count = halfSize + 4, cap = count;
        TestGraph graph(count + 1);
        for (auto [a,b] : halfEdges) boost::add_edge(a,b,graph);
        std::vector<size_t> boundary{top};
        boundary.insert(boundary.end(),order.begin(),order.end());
        boundary.insert(boundary.end(),{bottom,leftBottom,leftTop});
        std::vector<int> axis(count,-1);
        for (size_t i = 0; i < order.size() + 2; ++i) axis[boundary[i]] = static_cast<int>(i);
        for (size_t i = 0; i < boundary.size(); ++i) {
            boost::add_edge(boundary[i],boundary[(i+1)%boundary.size()],graph);
            boost::add_edge(cap,boundary[i],graph);
        }
        if (!planar(graph)) return {};

        // Complete a triangulated disk with this ordered axis on its boundary.
        // A private cap forces the prescribed boundary cycle to be cofacial.
        // No diagonal between nonconsecutive axis points is allowed: it would
        // overlap the straight axis after drawing. Only private edges are added.
        const size_t maximum = 3 * (count + 1) - 6;
        for (size_t a = 0; a < count && boost::num_edges(graph) < maximum; ++a)
            for (size_t b = a + 1; b < count && boost::num_edges(graph) < maximum; ++b) {
                if (boost::edge(a,b,graph).second || (axis[a] >= 0 && axis[b] >= 0 && std::abs(axis[a]-axis[b]) != 1)) continue;
                boost::add_edge(a,b,graph);
                if (!planar(graph)) boost::remove_edge(a,b,graph);
            }
        if (boost::num_edges(graph) != maximum) return {};

        Points positions(count);
        positions[top] = {0,-1}; positions[bottom] = {0,1};
        positions[leftTop] = {-1,-1}; positions[leftBottom] = {-1,1};
        std::map<size_t,size_t> midpointNeighbour(midpoints.begin(),midpoints.end());
        for (bool yCoordinate : {false,true}) {
            std::vector<size_t> free, index(count,count);
            for (size_t v = 0; v < halfSize; ++v)
                if (yCoordinate || axis[v] < 0) { index[v] = free.size(); free.push_back(v); }
            std::vector<std::vector<double>> matrix(free.size(),std::vector<double>(free.size()+1));
            for (size_t i = 0; i < free.size(); ++i) {
                const size_t v = free[i];
                auto add = [&](size_t u, double weight) {
                    matrix[i][i] += weight;
                    if (index[u] < free.size()) matrix[i][index[u]] -= weight;
                    else matrix[i].back() += weight * (yCoordinate ? positions[u].y : positions[u].x);
                };
                // Split crossing edges must remain straight when their private
                // axis midpoint is removed: its Y equals its paired endpoints.
                if (yCoordinate && midpointNeighbour.contains(v)) add(midpointNeighbour.at(v),1);
                else for (auto [it,end] = boost::adjacent_vertices(v,graph); it != end; ++it)
                    if (*it != cap) add(*it,yCoordinate && axis[v] >= 0 && axis[*it] < 0 ? 2 : 1);
            }
            const auto solution = solve(std::move(matrix));
            if (solution.size() != free.size()) return {};
            for (size_t i = 0; i < free.size(); ++i)
                (yCoordinate ? positions[free[i]].y : positions[free[i]].x) = solution[i];
        }
        Points drawing(n);
        for (size_t v = 0; v < n; ++v) if (halfIndex[v] < halfSize) {
            drawing[v] = positions[halfIndex[v]];
            drawing[partner[v]] = {-drawing[v].x,drawing[v].y};
        }
        if (!validDrawing(drawing,topology.edges)) return {};
        double minY = drawing[0].y, maxY = minY;
        for (auto p : drawing) { minY = std::min(minY,p.y); maxY = std::max(maxY,p.y); }
        for (auto& p : drawing) p.y -= (minY+maxY)/2;
        return drawing;
    }

    Points construct() {
        if (preferExistingFaces) return existingFacesDrawing();
        // Non-axis edges cannot cross the axis unless they join a mirror pair.
        // After removing such edges and fixed vertices, components must occur
        // in distinct exchanged pairs. This rejects involutions that cannot be
        // realized as a planar reflection, rather than mistaking them for axes.
        std::vector<size_t> component(n,n);
        size_t components = 0;
        for (size_t v = 0; v < n; ++v) if (partner[v] != v && component[v] == n) {
            std::vector<size_t> todo{v}; component[v] = components++;
            for (size_t i = 0; i < todo.size(); ++i) for (size_t u : neighbours[todo[i]])
                if (partner[u] != u && partner[todo[i]] != u && component[u] == n) {
                    component[u] = component[v]; todo.push_back(u);
                }
        }
        for (size_t v = 0; v < n; ++v) if (partner[v] != v && component[v] == component[partner[v]]) return {};
        std::vector<size_t> halfIndex(n,n);
        size_t halfSize = 0;
        for (size_t v = 0; v < n; ++v)
            if (partner[v] == v || component[v] < component[partner[v]]) halfIndex[v] = halfSize++;

        std::vector<std::vector<size_t>> fixedNeighbours(n);
        for (auto [a,b] : topology.edges) if (partner[a] == a && partner[b] == b) {
            fixedNeighbours[a].push_back(b); fixedNeighbours[b].push_back(a);
            if (fixedNeighbours[a].size() > 2 || fixedNeighbours[b].size() > 2) return {};
        }
        std::vector<bool> visited(n);
        std::vector<std::vector<size_t>> blocks;
        for (size_t v = 0; v < n; ++v) if (partner[v] == v && !visited[v] && fixedNeighbours[v].size() < 2) {
            std::vector<size_t> block;
            size_t at = v;
            while (at < n) {
                visited[at] = true; block.push_back(halfIndex[at]);
                size_t next = n;
                for (size_t u : fixedNeighbours[at]) if (!visited[u]) next = u;
                at = next;
            }
            blocks.push_back(std::move(block));
        }
        for (size_t v = 0; v < n; ++v) if (partner[v] == v && !visited[v]) return {}; // An axis cannot contain a cycle.
        std::vector<Edge> halfEdges;
        std::vector<std::pair<size_t,size_t>> midpoints;
        for (auto [a,b] : topology.edges) {
            if (partner[a] == b) {
                const size_t v = halfIndex[a] < n ? a : b;
                const size_t midpoint = halfSize++;
                midpoints.emplace_back(midpoint,halfIndex[v]);
                halfEdges.emplace_back(midpoint,halfIndex[v]); blocks.push_back({midpoint});
            } else if (halfIndex[a] < n && halfIndex[b] < n) halfEdges.emplace_back(halfIndex[a],halfIndex[b]);
        }
        // The sentinel must exceed private midpoint indices as well.
        for (auto& index : halfIndex) if (index == n) index = halfSize;
        std::vector<bool> used(blocks.size());
        std::vector<size_t> order;
        Points drawing;
        std::function<bool(size_t)> arrange = [&](size_t depth) {
            if (++states > 100000) throw NoReflection("Symmetry search limit reached; no compatible planar reflection was certified. Choose No symmetry or simplify the graph.");
            if (depth == blocks.size()) {
                if (order.size() > 1 && order.front() > order.back()) return false;
                drawing = halfDrawing(halfIndex,halfSize,halfEdges,midpoints,order);
                return !drawing.empty();
            }
            for (size_t b = 0; b < blocks.size(); ++b) if (!used[b]) {
                used[b] = true;
                const auto old = order.size();
                for (int reverse = 0; reverse < (blocks[b].size() > 1 ? 2 : 1); ++reverse) {
                    if (reverse) order.insert(order.end(),blocks[b].rbegin(),blocks[b].rend());
                    else order.insert(order.end(),blocks[b].begin(),blocks[b].end());
                    if (arrange(depth+1)) return true;
                    order.resize(old);
                }
                used[b] = false;
            }
            return false;
        };
        arrange(0);
        return drawing;
    }

    bool allowed(size_t a, size_t b) const {
        if (partner[b] != n || colors[a] != colors[b]) return false;
        for (size_t v = 0; v < n; ++v) if (partner[v] != n)
            if (adjacent[a][v] != adjacent[b][partner[v]] || adjacent[b][v] != adjacent[a][partner[v]]) return false;
        if (a == b) {
            size_t count = 0;
            for (size_t v : neighbours[a]) if (partner[v] == v && ++count > 2) return false;
            for (size_t v : neighbours[a]) if (partner[v] == v) {
                size_t degree = 1;
                for (size_t u : neighbours[v]) if (partner[u] == u && ++degree > 2) return false;
            }
        }
        return true;
    }

    bool search() {
        if (++states > (preferExistingFaces ? 10000u : 100000u)) throw NoReflection("Symmetry search limit reached; no compatible planar reflection was certified. Choose No symmetry or simplify the graph.");
        size_t at = n;
        std::vector<size_t> choices;
        for (size_t v = 0; v < n; ++v) if (partner[v] == n) {
            std::vector<size_t> candidates;
            for (size_t u = 0; u < n; ++u) if (u != v && allowed(v,u)) candidates.push_back(u);
            if (allowed(v,v)) candidates.push_back(v);
            if (candidates.empty()) return false;
            if (at == n || candidates.size() < choices.size()
                || (candidates.size() == choices.size() && neighbours[v].size() > neighbours[at].size())) {
                at = v; choices = std::move(candidates);
            }
        }
        if (at == n) {
            auto drawing = construct();
            if (drawing.empty()) return false;
            structuralReflection = true;
            auto reflection = std::make_shared<Reflection>();
            reflection->partner = partner; reflection->drawing = std::move(drawing);
            reflection->gridAligned = gridAligned;
            if (accept && !accept(*reflection)) { settingsRejected=true; return false; }
            matchingSettings=true;
            if (!compatibleAxis(*reflection)) return false;
            result = std::move(reflection);
            return true;
        }
        for (size_t u : choices) {
            partner[at] = u; partner[u] = at;
            if (search()) return true;
            partner[at] = partner[u] = n;
        }
        return false;
    }

public:
    Search(const Topology& graph, const PositionMagnetMap& magnets, double width, double height,
        bool gridAligned, const std::function<bool(const Reflection&)>& accept)
        : topology(graph), magnets(magnets), width(width), height(height), gridAligned(gridAligned), accept(accept), n(graph.ids.size()),
          adjacent(n,std::vector<bool>(n)), neighbours(n), partner(n,n), colors(n) {
        for (auto [a,b] : graph.edges) {
            adjacent[a][b] = adjacent[b][a] = true;
            neighbours[a].push_back(b); neighbours[b].push_back(a);
        }
        for (size_t v = 0; v < n; ++v) colors[v] = static_cast<int>(neighbours[v].size());
        for (size_t pass = 0; pass < n; ++pass) {
            std::map<std::vector<int>,int> classes;
            std::vector<int> refined(n);
            for (size_t v = 0; v < n; ++v) {
                std::vector<int> key;
                for (size_t u : neighbours[v]) key.push_back(colors[u]);
                std::sort(key.begin(),key.end()); key.insert(key.begin(),colors[v]);
                auto [it,added] = classes.emplace(key,static_cast<int>(classes.size()));
                refined[v] = it->second;
            }
            if (refined == colors) break;
            colors = std::move(refined);
        }
    }
    std::shared_ptr<const Reflection> run() {
        if (n == 0) return std::make_shared<Reflection>();
        // Failure to retain this particular embedding does not imply that the
        // graph has no planar reflection. Keep the existing bounded fallback.
        try { search(); } catch (const NoReflection&) {}
        if (result) return result;
        preferExistingFaces=false;states=tests=0;partner.assign(n,n);
        search();
        if (result) return result;
        if (settingsRejected && !matchingSettings) throw ConflictingSymmetrySettings("No reflection has matching paired zone and connection settings.");
        if (structuralReflection && gridAligned) throw ConflictingSymmetryMagnets("Complete symmetry needs magnet targets reflected about a grid axis through the map center: horizontal or vertical, or either diagonal on square maps. Move the magnets or enable Random magnet rotation.");
        if (structuralReflection) throw ConflictingSymmetryMagnets("The graph has reflection symmetry, but its saved magnet targets do not mirror about one line through the map center. Move or disable the conflicting magnets.");
        throw NoReflection("No planar reflection symmetry was found in the graph topology. Choose No symmetry or change the connections.");
    }
};
} // namespace

std::optional<TileAxis> tileAxisForNormal(DoubleVector2 normal,double width,double height) {
    const double length=std::hypot(normal.x,normal.y);
    if (!std::isfinite(length)||length==0) return {};
    normal.x/=length;normal.y/=length;
    if (std::abs(normal.y)<1e-6) return TileAxis::Vertical;
    if (std::abs(normal.x)<1e-6) return TileAxis::Horizontal;
    if (std::abs(width-height)<1e-8 && std::abs(std::abs(normal.x)-std::abs(normal.y))<1e-6)
        return normal.x*normal.y<0?TileAxis::MainDiagonal:TileAxis::AntiDiagonal;
    return {};
}

DoubleVector2 tileAxisNormal(TileAxis axis) {
    const double diagonal=std::sqrt(.5);
    switch(axis) {
        case TileAxis::Horizontal:return {0,1};
        case TileAxis::MainDiagonal:return {diagonal,-diagonal};
        case TileAxis::AntiDiagonal:return {diagonal,diagonal};
        default:return {1,0};
    }
}

IntVector2 reflectedTile(IntVector2 p,TileAxis axis,int width,int height) {
    switch(axis) {
        case TileAxis::Horizontal:return {p.x,height-1-p.y};
        case TileAxis::MainDiagonal:return {p.y,p.x};
        case TileAxis::AntiDiagonal:return {width-1-p.y,height-1-p.x};
        default:return {width-1-p.x,p.y};
    }
}

bool canonicalTile(IntVector2 p,IntVector2 q) {
    return p.y<q.y || (p.y==q.y && p.x<=q.x);
}

std::shared_ptr<const Reflection> findReflection(const Topology& topology,
    const PositionMagnetMap& magnets, double width, double height, bool gridAligned,
    const std::function<bool(const Reflection&)>& accept) {
    if (!std::isfinite(width) || !std::isfinite(height) || width <= 0 || height <= 0)
        throw std::invalid_argument("Symmetry needs finite positive map dimensions");
    return Search(topology,magnets,width,height,gridAligned,accept).run();
}
} // namespace planar
