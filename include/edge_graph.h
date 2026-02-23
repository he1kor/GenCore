#pragma once

#include "graph.h"
#include <unordered_map>
#include <stdexcept>
#include <format>

#include <functional>

template <hasID NodeT, typename SymEdgeT, typename AsymEdgeT>
class EdgeGraph : public Graph<NodeT>{
    public:
        EdgeGraph();
        EdgeGraph(const std::vector<Node<NodeT>>& nodes);
        EdgeGraph(const std::vector<std::pair<NodeT, std::vector<Identifiable>>>& rawGraph);
        EdgeGraph(
            const std::vector<std::pair<NodeT, std::vector<Identifiable>>>& rawGraph,
            const std::unordered_map<std::pair<Identifiable, Identifiable>, SymEdgeT, PairIDHash>& symEdges,
            const std::unordered_map<std::pair<Identifiable, Identifiable>, AsymEdgeT, AsymPairIDHash>& asymEdges
        );
        EdgeGraph(
            const std::vector<Node<NodeT>>& nodes, 
            const std::unordered_map<std::pair<Identifiable, Identifiable>, SymEdgeT, PairIDHash>& symEdges,
            const std::unordered_map<std::pair<Identifiable, Identifiable>, AsymEdgeT, AsymPairIDHash>& asymEdges
        );

        void initializeSymEdges(const std::unordered_map<std::pair<Identifiable, Identifiable>, SymEdgeT, PairIDHash>& symEdges);
        void initializeAsymEdges(const std::unordered_map<std::pair<Identifiable, Identifiable>, AsymEdgeT, AsymPairIDHash>& asymEdges);

        void separate(Identifiable id) override;
        
        void removeEdge(Identifiable node1, Identifiable node2) override;

        const SymEdgeT& getSymEdge(const NodeT& mainNode, const NodeT& neighbourNode) const;
        const AsymEdgeT& getAsymEdge(const NodeT& mainNode, const NodeT& neighbourNode) const;

        const std::unordered_map<std::pair<Identifiable, Identifiable>, SymEdgeT, PairIDHash>& getSymEdges() const;
        const std::unordered_map<std::pair<Identifiable, Identifiable>, AsymEdgeT, AsymPairIDHash>& getAsymEdges() const;
    protected:
    private:
        // Node order in pair doesn't matter
        std::unordered_map<std::pair<Identifiable, Identifiable>, SymEdgeT, PairIDHash> nodesToSymEdge;

        // Node order in pair matters
        std::unordered_map<std::pair<Identifiable, Identifiable>, AsymEdgeT, AsymPairIDHash> nodesToAsymEdge;

        void initializeEmptySymEdges();
        void initializeEmptyAsymEdges();
};



template <hasID NodeT, typename SymEdgeT, typename AsymEdgeT>
EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::EdgeGraph(){}

template <hasID NodeT, typename SymEdgeT, typename AsymEdgeT>
void EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::initializeEmptySymEdges(){
    if (!nodesToSymEdge.empty()){
        throw std::logic_error("Sym edges already initialized.");
    }
    for (Identifiable id1 : this->getIDs()){
        for (Identifiable id2 : this->getNode(id1).getNeighbours()){
            auto pair12 = std::make_pair(id1, id2);
            auto pair21 = std::make_pair(id2, id1);
            if (nodesToSymEdge.count(pair21)){
                continue;
            }
            nodesToSymEdge[pair12] = SymEdgeT();
        }
    }
}

template <hasID NodeT, typename SymEdgeT, typename AsymEdgeT>
void EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::initializeEmptyAsymEdges(){
    if (!nodesToAsymEdge.empty()){
        throw std::logic_error("Asym edges already initialized.");
    }
    for (Identifiable id1 : this->getIDs()){
        for (Identifiable id2 : this->getNode(id1).getNeighbours()){
            auto pair12 = std::make_pair(id1, id2);
            auto pair21 = std::make_pair(id2, id1);
            if (nodesToAsymEdge.count(pair21)){
                continue;
            }
            nodesToAsymEdge[pair12] = AsymEdgeT();
        }
    }
}

template <hasID NodeT, typename SymEdgeT, typename AsymEdgeT>
EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::EdgeGraph(
    const std::vector<std::pair<NodeT, std::vector<Identifiable>>>& rawGraph)
        : Graph<NodeT>(rawGraph){
            initializeEmptySymEdges();
            initializeEmptyAsymEdges();
        }

template <hasID NodeT, typename SymEdgeT, typename AsymEdgeT>
EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::EdgeGraph(
    const std::vector<Node<NodeT>>& nodes)
        : Graph<NodeT>(nodes){
            initializeEmptySymEdges();
            initializeEmptyAsymEdges();
        }
    
template<hasID NodeT, typename SymEdgeT, typename AsymEdgeT>
EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::EdgeGraph(
    const std::vector<std::pair<NodeT, std::vector<Identifiable>>>& rawGraph,
    const std::unordered_map<std::pair<Identifiable, Identifiable>, SymEdgeT, PairIDHash>& symEdges,
    const std::unordered_map<std::pair<Identifiable, Identifiable>, AsymEdgeT, AsymPairIDHash>& asymEdges
)
: Graph<NodeT>(rawGraph){
    initializeSymEdges(symEdges);
    initializeAsymEdges(asymEdges);
}

template <hasID NodeT, typename SymEdgeT, typename AsymEdgeT>
EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::EdgeGraph(
    const std::vector<Node<NodeT>>& nodes,
    const std::unordered_map<std::pair<Identifiable, Identifiable>, SymEdgeT, PairIDHash>& symEdges,
    const std::unordered_map<std::pair<Identifiable, Identifiable>, AsymEdgeT, AsymPairIDHash>& asymEdges
)
: Graph<NodeT>(nodes){
    initializeSymEdges(symEdges);
    initializeAsymEdges(asymEdges);
}

template <hasID NodeT, typename SymEdgeT, typename AsymEdgeT>
void EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::initializeSymEdges(
const std::unordered_map<std::pair<Identifiable, Identifiable>, SymEdgeT, PairIDHash>& edges){
    nodesToSymEdge.reserve(edges.size() * 2);
    for (const auto& [nodePair, edge] : edges){
        if (!this->getNeighbours(nodePair.first).contains(nodePair.second)){
            throw std::invalid_argument(std::format("Unexpected edge {} - {}", nodePair.first, nodePair.second));
        }
        nodesToSymEdge[PairIDHash::normalize(nodePair)] = edge;
    }
}

template <hasID NodeT, typename SymEdgeT, typename AsymEdgeT>
void EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::initializeAsymEdges(
const std::unordered_map<std::pair<Identifiable, Identifiable>, AsymEdgeT, AsymPairIDHash>& edges){
    nodesToAsymEdge = edges;
    for (const auto& [nodePair, edge] : edges){
        if (!this->getNeighbours(nodePair.first).contains(nodePair.second)){
            throw std::invalid_argument(std::format("Unexpected edge {} - {}", nodePair.first.toString(), nodePair.second.toString()));
        }
    }
}

template <hasID NodeT, typename SymEdgeT, typename AsymEdgeT>
void EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::removeEdge(Identifiable node1, Identifiable node2){
    Graph<NodeT>::removeEdge(node1, node2);
    nodesToSymEdge.erase({node1, node2});
    nodesToSymEdge.erase({node2, node1});
    nodesToAsymEdge.erase({node1, node2});
    nodesToAsymEdge.erase({node2, node1});
}

template <hasID NodeT, typename SymEdgeT, typename AsymEdgeT>
void EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::separate(Identifiable id){
    for (Identifiable neighbourID : this->getNeighbours(id)){
        nodesToSymEdge.erase({neighbourID, id});
        nodesToSymEdge.erase({id, neighbourID});
        nodesToAsymEdge.erase({id, neighbourID});
        nodesToAsymEdge.erase({neighbourID, id});
    }
    Graph<NodeT>::separate(id);
}


template <hasID NodeT, typename SymEdgeT, typename AsymEdgeT>
const SymEdgeT &EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::getSymEdge(const NodeT& node1, const NodeT& node2) const{
    return nodesToSymEdge.at(PairIDHash::normalize(std::make_pair(node1, node2)));
}

template <hasID NodeT, typename SymEdgeT, typename AsymEdgeT>
const AsymEdgeT &EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::getAsymEdge(const NodeT& mainNode, const NodeT& neighbourNode) const{
    return nodesToAsymEdge.at(mainNode, neighbourNode);
}

template <hasID NodeT, typename SymEdgeT, typename AsymEdgeT>
const std::unordered_map<std::pair<Identifiable, Identifiable>, SymEdgeT, PairIDHash>& EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::getSymEdges() const{
    return nodesToSymEdge;
}

template <hasID NodeT, typename SymEdgeT, typename AsymEdgeT>
const std::unordered_map<std::pair<Identifiable, Identifiable>, AsymEdgeT, AsymPairIDHash>& EdgeGraph<NodeT, SymEdgeT, AsymEdgeT>::getAsymEdges() const{
    return nodesToAsymEdge;
}
