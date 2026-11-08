#include "planar_layout.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/boyer_myrvold_planar_test.hpp>
#include <boost/graph/chrobak_payne_drawing.hpp>
#include <boost/graph/make_biconnected_planar.hpp>
#include <boost/graph/make_connected.hpp>
#include <boost/graph/make_maximal_planar.hpp>
#include <boost/graph/planar_canonical_ordering.hpp>
#include <algorithm>
#include <map>
#include <set>
#include <sstream>

namespace planar {
namespace {
using BoostGraph = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
    boost::no_property, boost::property<boost::edge_index_t, size_t>>;
using Descriptor = boost::graph_traits<BoostGraph>::edge_descriptor;
using Embedding = std::vector<std::vector<Descriptor>>;

void indexEdges(BoostGraph& graph) {
    size_t index = 0;
    for (auto [it, end] = boost::edges(graph); it != end; ++it)
        boost::put(boost::edge_index, graph, *it, index++);
}

bool embed(BoostGraph& graph, Embedding& embedding) {
    indexEdges(graph);
    embedding.assign(boost::num_vertices(graph), {});
    return boost::boyer_myrvold_planarity_test(boost::boyer_myrvold_params::graph = graph,
        boost::boyer_myrvold_params::embedding = embedding.data());
}

std::vector<std::vector<size_t>> faces(const BoostGraph& graph, const Embedding& embedding) {
    std::vector<std::vector<size_t>> rotation(embedding.size());
    for (size_t v = 0; v < embedding.size(); ++v) {
        for (auto e : embedding[v]) {
            auto a = boost::source(e, graph), b = boost::target(e, graph);
            rotation[v].push_back(a == v ? b : a);
        }
    }
    std::set<Edge> visited;
    std::vector<std::vector<size_t>> result;
    for (size_t u = 0; u < rotation.size(); ++u) {
        for (size_t v : rotation[u]) {
            if (visited.contains({u, v})) continue;
            std::vector<size_t> face;
            size_t a = u, b = v;
            do {
                if (!visited.emplace(a, b).second)
                    throw std::logic_error("Invalid planar rotation system");
                face.push_back(a);
                const auto& around = rotation[b];
                const auto index = std::find(around.begin(), around.end(), a) - around.begin();
                const size_t next = around[(index + around.size() - 1) % around.size()];
                a = b;
                b = next;
            } while (a != u || b != v);
            result.push_back(std::move(face));
        }
    }
    return result;
}
} // namespace

std::shared_ptr<const Topology> makeTopology(std::vector<Identifiable> ids,
    const std::vector<std::pair<Identifiable, Identifiable>>& inputEdges) {
    auto result = std::make_shared<Topology>();
    std::sort(ids.begin(), ids.end());
    if (std::adjacent_find(ids.begin(), ids.end()) != ids.end())
        throw std::invalid_argument("Duplicate node IDs in embedding graph");
    result->ids = ids;
    std::map<Identifiable, size_t> indices;
    for (size_t i = 0; i < ids.size(); ++i) indices.emplace(ids[i], i);
    std::set<Edge> unique;
    for (auto [a, b] : inputEdges) {
        if (!indices.contains(a) || !indices.contains(b))
            throw std::invalid_argument("Embedding edge references a missing node");
        if (a == b) throw std::invalid_argument("Embedding edges must have distinct endpoints");
        auto u = indices.at(a), v = indices.at(b);
        unique.emplace(std::min(u, v), std::max(u, v));
    }
    result->edges.assign(unique.begin(), unique.end());
    result->scaffoldSize = ids.size();
    if (ids.size() < 3) {
        result->scaffoldEdges = result->edges;
        for (size_t i = 0; i < ids.size(); ++i) result->gridDrawing.emplace_back(i, 0);
        return result;
    }

    BoostGraph graph(ids.size());
    for (auto [u, v] : result->edges) boost::add_edge(u, v, graph);
    indexEdges(graph);
    std::vector<Descriptor> obstruction;
    if (!boost::boyer_myrvold_planarity_test(boost::boyer_myrvold_params::graph = graph,
            boost::boyer_myrvold_params::kuratowski_subgraph = std::back_inserter(obstruction))) {
        std::ostringstream message;
        message << "a planar zone graph; nonplanar obstruction contains edges";
        for (auto e : obstruction)
            message << " (" << ids[boost::source(e, graph)].getID() << ", "
                << ids[boost::target(e, graph)].getID() << ')';
        throw NonPlanarGraph(message.str());
    }

    // Augment only the private drawing copy. No added edge reaches zone growth.
    boost::make_connected(graph);
    Embedding embedding;
    if (!embed(graph, embedding)) throw std::logic_error("Connected augmentation lost planarity");
    boost::make_biconnected_planar(graph, embedding.data());
    if (!embed(graph, embedding)) throw std::logic_error("Biconnected augmentation lost planarity");
    result->rotation.resize(ids.size());
    for (size_t v = 0; v < ids.size(); ++v) for (auto edge : embedding[v]) {
        const size_t a = boost::source(edge, graph), b = boost::target(edge, graph);
        const size_t u = a == v ? b : a;
        if (unique.contains({std::min(v,u),std::max(v,u)})) result->rotation[v].push_back(u);
    }
    auto faceCycles = faces(graph, embedding);
    const size_t outer = std::max_element(faceCycles.begin(), faceCycles.end(),
        [](const auto& a, const auto& b) { return a.size() < b.size(); }) - faceCycles.begin();
    auto perimeter = faceCycles[outer];
    std::reverse(perimeter.begin(), perimeter.end());
    result->outerFace = perimeter;
    std::set<Edge> scaffold;
    const auto add = [&](size_t a, size_t b) { scaffold.emplace(std::min(a, b), std::max(a, b)); };
    for (auto [it, end] = boost::edges(graph); it != end; ++it)
        add(boost::source(*it, graph), boost::target(*it, graph));
    for (size_t f = 0; f < faceCycles.size(); ++f) {
        if (f == outer || faceCycles[f].size() == 3) continue;
        // A face center avoids choosing a preferred diagonal in grid cells.
        const size_t center = result->scaffoldSize++;
        for (size_t v : faceCycles[f]) add(center, v);
    }
    // A virtual surrounding frame lets every real node move. Pinning real
    // boundary nodes would prevent their edge preferences from being fitted.
    const size_t frameStart = result->scaffoldSize;
    result->scaffoldSize += perimeter.size();
    for (size_t i = 0; i < perimeter.size(); ++i) {
        const size_t j = (i + 1) % perimeter.size();
        result->boundary.push_back(frameStart + i);
        add(frameStart + i, frameStart + j);
        add(frameStart + i, perimeter[i]);
        add(frameStart + i, perimeter[j]);
    }
    result->scaffoldEdges.assign(scaffold.begin(), scaffold.end());

    // Integer straight-line construction is an independent fallback for badly
    // conditioned harmonic drawings (for example, nested triangulations).
    boost::make_maximal_planar(graph, embedding.data());
    if (!embed(graph, embedding)) throw std::logic_error("Triangulation lost planarity");
    std::vector<size_t> ordering;
    auto embeddingMap = boost::make_iterator_property_map(embedding.begin(), boost::get(boost::vertex_index, graph));
    boost::planar_canonical_ordering(graph, embeddingMap, std::back_inserter(ordering));
    struct GridPoint { size_t x = 0, y = 0; };
    std::vector<GridPoint> grid(ids.size());
    auto drawing = boost::make_iterator_property_map(grid.begin(), boost::get(boost::vertex_index, graph));
    boost::chrobak_payne_straight_line_drawing(graph, embeddingMap, ordering.begin(), ordering.end(), drawing);
    for (const auto& p : grid) result->gridDrawing.emplace_back(p.x, p.y);
    if (!validDrawing(result->gridDrawing, result->edges))
        throw std::logic_error("Planar construction returned an invalid drawing");
    return result;
}
} // namespace planar
