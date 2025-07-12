#include "graph.h"
#include <unordered_map>

#include <functional>

template<typename NodeT, typename EdgeT>
class EdgeGraph : public Graph{
    static_assert(std::is_base_of_v<Identifiable, NodeT>, "NodeT must inherit from Identifiable");
    static_assert(std::is_base_of_v<Identifiable, EdgeT>, "EdgeT must inherit from Identifiable");
    public:
        EdgeGraph();
        EdgeGraph(
            const std::vector<std::pair<NodeT, std::vector<Identifiable>>>& rawGraph,
            const std::set<std::pair<Identifiable, Identifiable>, EdgeT>& edges
        );
        EdgeGraph(
            const std::vector<Node<NodeT>>& nodes, 
            const std::set<std::pair<Identifiable, Identifiable>, EdgeT>& edges);

        void initializeEdges(const std::set<std::pair<Identifiable, Identifiable>, EdgeT>& edges);
        void separate(Identifiable id) override;
        void removeEdge(Identifiable node1, Identifiable node2) override;
        Identifiable getEdgeID(Identifiable node1, Identifiable node2) const;
    protected:
        EdgeGraph(const std::vector<std::pair<NodeT, std::vector<Identifiable>>>& rawGraph);
        EdgeGraph(const std::vector<Node<NodeT>>& nodes);
    private:
        std::unordered_map<std::pair<Identifiable, Identifiable>, Identifiable, PairIDHash> edgeIDs;
        std::unordered_map<Identifiable, EdgeT> edges;
        void initializeEdgeIDs();
};



template<typename NodeT, typename edgeT>
EdgeGraph<NodeT, edgeT>::EdgeGraph(){}

template<typename NodeT, typename edgeT>
void EdgeGraph<NodeT, edgeT>::initializeEdgeIDs(){
    int max_edge_id = 0;
    for (Node node1 : nodes){
        for (Node node2 : node1.getNeighbours()){
            if (edgeIDs.count(std::make_pair(node2.getID(), node1.getID()))){
                edgeIDs[std::make_pair(node1.getID(), node2.getID())] = edgeIDs.at(std::make_pair(node2.getID(), node1.getID()));
                continue;
            }
            edgeIDs[std::make_pair(node1.getID(), node2.getID())] = max_edge_id;

            edges[max_edge_id] = edgeT();
            max_edge_id++;
        }
    }
    for ()
}

template<typename NodeT, typename edgeT>
EdgeGraph<NodeT, edgeT>::EdgeGraph(
    const std::vector<std::pair<NodeT, std::vector<Identifiable>>>& rawGraph) : Graph(idGraph){}

template <typename NodeT, typename EdgeT>
EdgeGraph<NodeT, EdgeT>::EdgeGraph(
    const std::vector<std::pair<NodeT, std::vector<Identifiable>>> &rawGraph,
    const std::set<std::pair<Identifiable, Identifiable>, EdgeT>& edges){
    EdgeGraph<NodeT, EdgeT>(rawGraph);
    initializeEdges(edges);
}
    
template <typename NodeT, typename EdgeT>
EdgeGraph<NodeT, EdgeT>::EdgeGraph(
    const std::vector<Node<NodeT>>& nodes,
    const std::set<std::pair<Identifiable, Identifiable>, EdgeT>& edges){
    EdgeGraph<NodeT, EdgeT>(nodes);
}

template <typename NodeT, typename EdgeT>
void EdgeGraph<NodeT, EdgeT>::initializeEdges(
    const std::set<std::pair<Identifiable, Identifiable>, EdgeT> &edges){
    this->edges(edges.begin(), edges.end());
}

template<typename NodeT, typename edgeT>
EdgeGraph<NodeT, edgeT>::EdgeGraph(const std::vector<Node<NodeT>>& nodes) : Graph(nodes){}

template<typename NodeT, typename edgeT>
void EdgeGraph<NodeT, edgeT>::removeEdge(Identifiable node1, Identifiable node2){
    Graph::removeEdge(node1, node2);
    edgeIDs.erase({node1, node2});
    edgeIDs.erase({node2, node1});
}

template<typename NodeT, typename edgeT>
Identifiable EdgeGraph<NodeT, edgeT>::getEdgeID(Identifiable node1, Identifiable node2) const{
    return edgeIDs.at({node1, node2});
}

template <typename NodeT, typename edgeT>
void EdgeGraph<NodeT, edgeT>::separate(Identifiable id)
{
    for (int neighbourID : getNeighbours(id)){
        edgeIDs.erase({neighbourID, id});
        edgeIDs.erase({id, neighbourID});
    }
    Graph::separate(id);
}