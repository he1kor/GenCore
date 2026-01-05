#pragma once

#include "graph.h"
#include <unordered_map>
#include <stdexcept>

#include <functional>

template <typename NodeT, hasID SymEdgeT = Identifiable, hasID AsymEdgeT = Identifiable>
class EdgeGraph : public Graph<NodeT>{
    public:
        EdgeGraph();
        EdgeGraph(const std::vector<Node<NodeT>>& nodes);
        EdgeGraph(const std::vector<std::pair<NodeT, std::vector<Identifiable>>>& rawGraph);
        EdgeGraph(
            const std::vector<std::pair<NodeT, std::vector<Identifiable>>>& rawGraph,
            const std::unordered_map<std::pair<Identifiable, Identifiable>, SymEdgeT, PairIDHash>& edges
        );
        EdgeGraph(
            const std::vector<Node<NodeT>>& nodes, 
            const std::unordered_map<std::pair<Identifiable, Identifiable>, SymEdgeT, PairIDHash>& edges);

        void initializeEdges(const std::unordered_map<std::pair<Identifiable, Identifiable>, SymEdgeT, PairIDHash>& edges);
        void separate(Identifiable id) override;
        void removeEdge(Identifiable node1, Identifiable node2) override;
        Identifiable getSymEdgeID(Identifiable node1, Identifiable node2) const;
        const SymEdgeT& getSymEdge(Identifiable edgeID) const;
        const SymEdgeT& getSymEdge(Identifiable node1, Identifiable node2) const;
        std::pair<Identifiable, Identifiable> getSymEdgeNodeIDs(Identifiable edgeID) const;
        std::pair<NodeT, NodeT> getSymEdgeNodes(Identifiable edgeID) const;
        const std::vector<Identifiable>& getSymEdgeIDs() const;
    protected:
    private:
        std::unordered_map<std::pair<Identifiable, Identifiable>, Identifiable, PairIDHash> nodesToEdge;
        std::unordered_map<Identifiable, std::pair<Identifiable, Identifiable>, IDHash> edgeToNodes;
        std::unordered_map<Identifiable, SymEdgeT, IDHash> symEdges;
        std::vector<Identifiable> symEdgeIDs;
        void initializeEmptyEdges();
};



template <typename NodeT, hasID SymEdgeT, hasID AsymEdgeT>
EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::EdgeGraph(){}

template <typename NodeT, hasID SymEdgeT, hasID AsymEdgeT>
void EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::initializeEmptyEdges(){
    if (nodesToEdge.size() != 0){
        throw std::logic_error("Edges already initialized.");
    }
    int maxEdgeID = 0;
    for (Identifiable id : this->getIDs()){
        auto node1 = this->getNode(id);
        for (Identifiable id2 : node1.getNeighbours()){
            const Node<NodeT>& node2 = this->getNode(id2);
            if (nodesToEdge.count(std::make_pair(node2.getID(), node1.getID()))){
                nodesToEdge[std::make_pair(node1.getID(), node2.getID())] = nodesToEdge.at(std::make_pair(node2.getID(), node1.getID()));
                continue;
            }
            edgeToNodes[maxEdgeID] = std::make_pair(node1.getID(), node2.getID());
            nodesToEdge[std::make_pair(node1.getID(), node2.getID())] = maxEdgeID;

            symEdges[maxEdgeID] = SymEdgeT();
            symEdgeIDs.push_back(static_cast<Identifiable>(maxEdgeID));
            maxEdgeID++;
        }
    }
}

template <typename NodeT, hasID SymEdgeT, hasID AsymEdgeT>
EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::EdgeGraph(
    const std::vector<std::pair<NodeT, std::vector<Identifiable>>>& rawGraph)
        : Graph<NodeT>(rawGraph){
            initializeEmptyEdges();
        }

template <typename NodeT, hasID SymEdgeT, hasID AsymEdgeT>
EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::EdgeGraph(
    const std::vector<Node<NodeT>>& nodes)
        : Graph<NodeT>(nodes){
            initializeEmptyEdges();
        }
    
template <typename NodeT, hasID SymEdgeT, hasID AsymEdgeT>
EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::EdgeGraph(
    const std::vector<std::pair<NodeT, std::vector<Identifiable>>>& rawGraph,
    const std::unordered_map<std::pair<Identifiable, Identifiable>, SymEdgeT, PairIDHash>& edges)
        : Graph<NodeT>(rawGraph){
            initializeEdges(edges);
        }

template <typename NodeT, hasID SymEdgeT, hasID AsymEdgeT>
EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::EdgeGraph(
    const std::vector<Node<NodeT>>& nodes,
    const std::unordered_map<std::pair<Identifiable, Identifiable>, SymEdgeT, PairIDHash>& edges)
        : Graph<NodeT>(nodes){
            initializeEdges(edges);
        }

template <typename NodeT, hasID SymEdgeT, hasID AsymEdgeT>
void EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::initializeEdges(
    const std::unordered_map<std::pair<Identifiable, Identifiable>, SymEdgeT, PairIDHash>& edges){
        nodesToEdge.reserve(edges.size() * 2);
        edgeToNodes.reserve(edges.size());
        symEdges.reserve(edges.size());
        symEdgeIDs.reserve(edges.size());
        for (const auto& [nodePair, edge] : edges){
            nodesToEdge[nodePair] = static_cast<Identifiable>(edge);
            nodesToEdge[std::make_pair(nodePair.second, nodePair.first)] = static_cast<Identifiable>(edge); //TODO:: make nodesToEdge store only N pairs instead of 2*N with reversed pairs;
            edgeToNodes[static_cast<Identifiable>(edge)] = nodePair;
            symEdges[static_cast<Identifiable>(edge)] = edge;
            symEdgeIDs.push_back(static_cast<Identifiable>(edge));
        }
    }

template <typename NodeT, hasID SymEdgeT, hasID AsymEdgeT>
void EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::removeEdge(Identifiable node1, Identifiable node2){
    Graph<NodeT>::removeEdge(node1, node2);
    
    edgeToNodes.erase(nodesToEdge.at({node1, node2}));

    nodesToEdge.erase({node1, node2});
    nodesToEdge.erase({node2, node1});
}

template <typename NodeT, hasID SymEdgeT, hasID AsymEdgeT>
Identifiable EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::getSymEdgeID(Identifiable node1, Identifiable node2) const{
    return nodesToEdge.at({node1, node2});
}

template <typename NodeT, hasID SymEdgeT, hasID AsymEdgeT>
const SymEdgeT &EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::getSymEdge(Identifiable symEdgeID) const{
    return symEdges[symEdgeID];
}

template <typename NodeT, hasID SymEdgeT, hasID AsymEdgeT>
const SymEdgeT &EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::getSymEdge(Identifiable node1, Identifiable node2) const{
    return getEdge(getSymEdgeID(node1, node2));
}

template <typename NodeT, hasID SymEdgeT, hasID AsymEdgeT>
std::pair<Identifiable, Identifiable> EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::getSymEdgeNodeIDs(Identifiable symEdgeID) const{
    return edgeToNodes.at(symEdgeID);
}

template <typename NodeT, hasID SymEdgeT, hasID AsymEdgeT>
std::pair<NodeT, NodeT> EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::getSymEdgeNodes(Identifiable symEdgeID) const{
    auto [id1, id2] = getSymEdgeNodeIDs(symEdgeID);
    return {this->getNode(id1), this->getNode(id2)};
}

template <typename NodeT, hasID SymEdgeT, hasID AsymEdgeT>
const std::vector<Identifiable> &EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::getSymEdgeIDs() const{
    return symEdgeIDs;
}

template <typename NodeT, hasID SymEdgeT, hasID AsymEdgeT>
void EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::separate(Identifiable id){
    for (Identifiable neighbourID : this->getNeighbours(id)){
        edgeToNodes.erase(nodesToEdge.at({id, neighbourID}));

        nodesToEdge.erase({neighbourID, id});
        nodesToEdge.erase({id, neighbourID});
    }
    this->separate(id);
}