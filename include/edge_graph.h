#pragma once

#include "graph.h"
#include <unordered_map>
#include <stdexcept>

#include <functional>

template <typename NodeT, typename EdgeT = Identifiable>
requires hasID<EdgeT>
class EdgeGraph : public Graph<NodeT>{
    public:
        EdgeGraph();
        EdgeGraph(const std::vector<Node<NodeT>>& nodes);
        EdgeGraph(const std::vector<std::pair<NodeT, std::vector<Identifiable>>>& rawGraph);
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
        std::pair<Identifiable, Identifiable> getEdgeNodeIDs(Identifiable edgeID) const;
        std::pair<NodeT, NodeT> getEdgeNodes(Identifiable edgeID) const;
        const std::vector<Identifiable>& getEdgeIDs() const;
    protected:
    private:
        std::unordered_map<std::pair<Identifiable, Identifiable>, Identifiable, PairIDHash> nodesToEdge;
        std::unordered_map<Identifiable, std::pair<Identifiable, Identifiable>, IDHash> edgeToNodes;
        std::unordered_map<Identifiable, EdgeT, IDHash> edges;
        std::vector<Identifiable> edgeIDs;
        void initializeEmptyEdges();
};



template <typename NodeT, typename EdgeT>
requires hasID<EdgeT>
EdgeGraph<NodeT, EdgeT>::EdgeGraph(){}

template<typename NodeT, typename EdgeT>
requires hasID<EdgeT>
void EdgeGraph<NodeT, EdgeT>::initializeEmptyEdges(){
    if (nodesToEdge.size() != 0){
        throw std::logic_error("Edges already initialized.");
    }
    int max_edge_id = 0;
    for (Identifiable id : this->getIDs()){
        auto node1 = this->getNode(id);
        for (Identifiable id2 : node1.getNeighbours()){
            const Node<NodeT>& node2 = this->getNode(id2);
            if (nodesToEdge.count(std::make_pair(node2.getID(), node1.getID()))){
                nodesToEdge[std::make_pair(node1.getID(), node2.getID())] = nodesToEdge.at(std::make_pair(node2.getID(), node1.getID()));
                continue;
            }
            edgeToNodes[max_edge_id] = std::make_pair(node1.getID(), node2.getID());
            nodesToEdge[std::make_pair(node1.getID(), node2.getID())] = max_edge_id;

            edges[max_edge_id] = EdgeT();
            edgeIDs.push_back(static_cast<Identifiable>(max_edge_id));
            max_edge_id++;
        }
    }
}

template<typename NodeT, typename EdgeT>
requires hasID<EdgeT>
EdgeGraph<NodeT, EdgeT>::EdgeGraph(
    const std::vector<std::pair<NodeT, std::vector<Identifiable>>>& rawGraph)
        : Graph<NodeT>(rawGraph){
            initializeEmptyEdges();
        }

template<typename NodeT, typename EdgeT>
requires hasID<EdgeT>
EdgeGraph<NodeT, EdgeT>::EdgeGraph(
    const std::vector<Node<NodeT>>& nodes)
        : Graph<NodeT>(nodes){
            initializeEmptyEdges();
        }
    
template <typename NodeT, typename EdgeT>
requires hasID<EdgeT>
EdgeGraph<NodeT, EdgeT>::EdgeGraph(
    const std::vector<std::pair<NodeT, std::vector<Identifiable>>>& rawGraph,
    const std::unordered_map<std::pair<Identifiable, Identifiable>, EdgeT, PairIDHash>& edges)
        : Graph<NodeT>(rawGraph){
            initializeEdges(edges);
        }

template <typename NodeT, typename EdgeT>
requires hasID<EdgeT>
EdgeGraph<NodeT, EdgeT>::EdgeGraph(
    const std::vector<Node<NodeT>>& nodes,
    const std::unordered_map<std::pair<Identifiable, Identifiable>, EdgeT, PairIDHash>& edges)
        : Graph<NodeT>(nodes){
            initializeEdges(edges);
        }

template <typename NodeT, typename EdgeT>
requires hasID<EdgeT>
void EdgeGraph<NodeT, EdgeT>::initializeEdges(
    const std::unordered_map<std::pair<Identifiable, Identifiable>, EdgeT, PairIDHash>& edges){
        nodesToEdge.reserve(edges.size() * 2);
        edgeToNodes.reserve(edges.size());
        edges.reserve(edges.size());
        edgeIDs.reserve(edges.size());
        for (const auto& [nodePair, edge] : edges){
            nodesToEdge[nodePair] = static_cast<Identifiable>(edge);
            nodesToEdge[std::make_pair(nodePair.second, nodePair.first)] = static_cast<Identifiable>(edge); //TODO:: make nodesToEdge store only N pairs instead of 2*N with reversed pairs;
            edgeToNodes[static_cast<Identifiable>(edge)] = nodePair;
            edges[static_cast<Identifiable>(edge)] = edge;
            edgeIDs.push_back(static_cast<Identifiable>(edge));
        }
    }

template<typename NodeT, typename EdgeT>
requires hasID<EdgeT>
void EdgeGraph<NodeT, EdgeT>::removeEdge(Identifiable node1, Identifiable node2){
    Graph<NodeT>::removeEdge(node1, node2);
    
    edgeToNodes.erase(nodesToEdge.at({node1, node2}));

    nodesToEdge.erase({node1, node2});
    nodesToEdge.erase({node2, node1});
}

template<typename NodeT, typename EdgeT>
requires hasID<EdgeT>
Identifiable EdgeGraph<NodeT, EdgeT>::getEdgeID(Identifiable node1, Identifiable node2) const{
    return nodesToEdge.at({node1, node2});
}

template <typename NodeT, typename EdgeT>
requires hasID<EdgeT>
const EdgeT &EdgeGraph<NodeT, EdgeT>::getEdge(Identifiable edgeID) const{
    return edges[edgeID];
}

template <typename NodeT, typename EdgeT>
requires hasID<EdgeT>
const EdgeT &EdgeGraph<NodeT, EdgeT>::getEdge(Identifiable node1, Identifiable node2) const{
    return getEdge(getEdgeID(node1, node2));
}

template <typename NodeT, typename EdgeT>
requires hasID<EdgeT>
std::pair<Identifiable, Identifiable> EdgeGraph<NodeT, EdgeT>::getEdgeNodeIDs(Identifiable edgeID) const{
    return edgeToNodes.at(edgeID);
}

template <typename NodeT, typename EdgeT>
requires hasID<EdgeT>
std::pair<NodeT, NodeT> EdgeGraph<NodeT, EdgeT>::getEdgeNodes(Identifiable edgeID) const{
    auto [id1, id2] = getEdgeNodeIDs(edgeID);
    return {this->getNode(id1), this->getNode(id2)};
}

template <typename NodeT, typename EdgeT>
requires hasID<EdgeT>
const std::vector<Identifiable> &EdgeGraph<NodeT, EdgeT>::getEdgeIDs() const{
    return edgeIDs;
}

template <typename NodeT, typename EdgeT>
requires hasID<EdgeT>
void EdgeGraph<NodeT, EdgeT>::separate(Identifiable id){
    for (Identifiable neighbourID : this->getNeighbours(id)){
        edgeToNodes.erase(nodesToEdge.at({id, neighbourID}));

        nodesToEdge.erase({neighbourID, id});
        nodesToEdge.erase({id, neighbourID});
    }
    this->separate(id);
}