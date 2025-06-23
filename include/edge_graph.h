#include "graph.h"
#include <unordered_map>

#include <functional>

struct PairHash {
    std::size_t operator()(const std::pair<int, int>& e) const {
        auto a = std::min(e.first, e.second);
        auto b = std::max(e.first, e.second);
        return std::hash<int>{}(a) ^ (std::hash<int>{}(b) << 1);
    }
};

class EdgeGraph : public Graph{
    public:
        EdgeGraph();
        EdgeGraph(std::vector<std::vector<int>> idGraph);
        EdgeGraph(std::vector<Node> nodes);
        void separate(int id) override;
        void removeEdge(int node1, int node2) override;
        int getEdgeID(int node1, int node2) const;
    private:
        std::unordered_map<std::pair<int, int>, int, PairHash> edgeIDs;
        void initializeEdgeIDs();
};