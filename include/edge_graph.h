#pragma once

#include "graph.h"
#include <unordered_map>

#include <functional>

template<typename NodeT, typename EdgeT>
class EdgeGraph : public Graph<NodeT>{
    static_assert(std::is_base_of_v<Identifiable, NodeT>, "NodeT must inherit from Identifiable");
    static_assert(std::is_base_of_v<Identifiable, EdgeT>, "EdgeT must inherit from Identifiable");
    public:
        EdgeGraph();
        EdgeGraph(
            const std::vector<std::pair<NodeT, std::vector<Identifiable>>>& rawGraph,
            const std::unordered_map<std::pair<Identifiable, Identifiable>, EdgeT, PairIDHash>& edges
        );
        EdgeGraph(
            const std::vector<Node<NodeT>>& nodes, 
            const std::unordered_map<std::pair<Identifiable, Identifiable>, EdgeT, PairIDHash>& edges);

        void initializeEdges(const std::unordered_map<std::pair<Identifiable, Identifiable>, EdgeT, PairIDHash>& edges);
        void separate(Identifiable id) override;
        void removeEdge(Identifiable node1, Identifiable node2) override;
        Identifiable getEdgeID(Identifiable node1, Identifiable node2) const;
        const EdgeT& getEdge(Identifiable edgeID) const;
        const EdgeT& getEdge(Identifiable node1, Identifiable node2) const;
        EdgeGraph(const std::vector<std::pair<NodeT, std::vector<Identifiable>>>& rawGraph);
        EdgeGraph(const std::vector<Node<NodeT>>& nodes);
    protected:
    private:
        std::unordered_map<std::pair<Identifiable, Identifiable>, Identifiable, PairIDHash> edgeIDs;
        std::unordered_map<Identifiable, EdgeT, IDHash> edges;
        void initializeEdgeIDs();
};



template<typename NodeT, typename EdgeT>
EdgeGraph<NodeT, EdgeT>::EdgeGraph(){}

template<typename NodeT, typename EdgeT>
void EdgeGraph<NodeT, EdgeT>::initializeEdgeIDs(){
    if (edgeIDs.size() != 0){
        throw std::logic_error("Edges already initialized.");
    }
    int max_edge_id = 0;
    for (Identifiable id : this->getIDs()){
        auto node1 = this->getNode(id);
        for (Identifiable id2 : node1.getNeighbours()){
            const Node<NodeT>& node2 = this->getNode(id2);
            if (edgeIDs.count(std::make_pair(node2.getID(), node1.getID()))){
                edgeIDs[std::make_pair(node1.getID(), node2.getID())] = edgeIDs.at(std::make_pair(node2.getID(), node1.getID()));
                continue;
            }
            edgeIDs[std::make_pair(node1.getID(), node2.getID())] = max_edge_id;

            edges[max_edge_id] = EdgeT();
            max_edge_id++;
        }
    }
}

template<typename NodeT, typename EdgeT>
EdgeGraph<NodeT, EdgeT>::EdgeGraph(
    const std::vector<std::pair<NodeT, std::vector<Identifiable>>>& rawGraph) : Graph<NodeT>(rawGraph){
        initializeEdgeIDs();
    }

template <typename NodeT, typename EdgeT>
EdgeGraph<NodeT, EdgeT>::EdgeGraph(
    const std::vector<std::pair<NodeT, std::vector<Identifiable>>> &rawGraph,
    const std::unordered_map<std::pair<Identifiable, Identifiable>, EdgeT, PairIDHash>& edges) : EdgeGraph<NodeT, EdgeT>(rawGraph){
    initializeEdges(edges);
}

template <typename NodeT, typename EdgeT>
EdgeGraph<NodeT, EdgeT>::EdgeGraph(
    const std::vector<Node<NodeT>>& nodes,
    const std::unordered_map<std::pair<Identifiable, Identifiable>, EdgeT, PairIDHash>& edges) : EdgeGraph<NodeT, EdgeT>(nodes){
        initializeEdges(edges);
    }

template <typename NodeT, typename EdgeT>
void EdgeGraph<NodeT, EdgeT>::initializeEdges(
    const std::unordered_map<std::pair<Identifiable, Identifiable>, EdgeT, PairIDHash> &edges){
    for (const auto& [nodePair, edge] : edges){
        edgeIDs[nodePair] = edge;
        edges[static_cast<Identifiable>(edge)] = edge;
    }
}

template<typename NodeT, typename EdgeT>
EdgeGraph<NodeT, EdgeT>::EdgeGraph(const std::vector<Node<NodeT>>& nodes) : Graph<NodeT>(nodes){
    initializeEdgeIDs();
}

template<typename NodeT, typename EdgeT>
void EdgeGraph<NodeT, EdgeT>::removeEdge(Identifiable node1, Identifiable node2){
    this->removeEdge(node1, node2);
    edgeIDs.erase({node1, node2});
    edgeIDs.erase({node2, node1});
}

template<typename NodeT, typename EdgeT>
Identifiable EdgeGraph<NodeT, EdgeT>::getEdgeID(Identifiable node1, Identifiable node2) const{
    return edgeIDs.at({node1, node2});
}

template <typename NodeT, typename EdgeT>
const EdgeT &EdgeGraph<NodeT, EdgeT>::getEdge(Identifiable edgeID) const{
    return edges[edgeID];
}

template <typename NodeT, typename EdgeT>
const EdgeT &EdgeGraph<NodeT, EdgeT>::getEdge(Identifiable node1, Identifiable node2) const{
    return getEdge(getEdgeID(node1, node2));
}

template <typename NodeT, typename EdgeT>
void EdgeGraph<NodeT, EdgeT>::separate(Identifiable id){
    for (Identifiable neighbourID : this->getNeighbours(id)){
        edgeIDs.erase({neighbourID, id});
        edgeIDs.erase({id, neighbourID});
    }
    this->separate(id);
}