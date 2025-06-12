#include "graph.h"
#include <unordered_map>

class EdgeGraph : public Graph{
    public:
        EdgeGraph();
        EdgeGraph(std::vector<std::vector<int>> idGraph);
        EdgeGraph(std::vector<Node> nodes);
        void separate(int id) override;
        void removeEdge(int node1, int node2) override;
    private:
        std::unordered_map<std::pair<int, int>, int> edgeIDs;
        void initializeEdgeIDs();
};