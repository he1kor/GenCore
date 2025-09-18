#pragma once

#include <vector>
#include <array>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

#include "2d.h"
#include "grid.h"

namespace tiles{
    class Border{
    public:
        class Segment;
        struct Edge{
            Edge(IntVector2 aTile, IntVector2 bTile);
            Edge(IntVector2 topLeftPos, Orientation orientation)
                : topLeftPos(topLeftPos), orientation(orientation){}
            Edge(const Segment& segment);
            IntVector2 getTopLeftTile() const;
            IntVector2 getbottomRightTile() const;
            IntVector2 topLeftPos;
            Orientation orientation;
            bool operator==(const tiles::Border::Edge& other) const = default;
            struct Hash{
                std::size_t operator()(const Edge& edge) const;
            };
        };
        class Segment{
        public:
            friend Edge::Edge(const Segment& segment);
            Segment() = default;
            Segment(IntVector2 aTile, IntVector2 bTile);
            Segment(IntVector2 startPos, Direction direction) 
                : startPos(startPos), direction(direction){}
            Segment(const Edge& edge);
            void turnLeft();
            void turnRight();
            void turnBackward();
            IntVector2 getLeftPos() const;
            IntVector2 getRightPos() const;

            void moveForward();
            bool fits(Edge& edge) const;
            std::array<Segment, 6> getNeighbours() const;
            std::array<Segment, 3> getNext() const;
            std::array<Segment, 3> getPrev() const;
            void setPos(IntVector2 pos);
            void setDirection(Direction direction);
            Direction getDirection() const{return direction;};

            struct Hash{
                std::size_t operator()(const Segment& segment) const;
            };
            IntVector2 getNextPos() const;
            const IntVector2& getStartPos() const{return startPos;};
        private:
            IntVector2 startPos;
            Direction direction;
        };

        struct PossiblePass{
            size_t firstIndex;
            size_t secondIndex;
            size_t width;
        };

        Border() = default;

        //assumes distance between end of the i1 and begin of i2
        size_t manhattanFromTo(size_t i1, size_t i2) const;

        template <typename T>
        static std::unordered_map<std::pair<Identifiable, Identifiable>, std::vector<Border>, PairIDHash> getAllBorders(const Grid<T>& matrix);

        void initPassData(size_t minPassWidth, size_t maxPassWidth);

        const Segment& at(size_t i) const;
        const std::vector<Segment>& getSegments() const{return segments;};
        std::vector<Border::PossiblePass> possiblePasses(size_t index) const;
        size_t size() const{return segments.size();};
        IntVector2 getLeft(size_t i);
        IntVector2 getRight(size_t i);
    private:

        template <typename T>
        Border(std::unordered_set<tiles::Border::Edge, tiles::Border::Edge::Hash>& freeBorderEdges, std::unordered_set<tiles::Border::Edge, tiles::Border::Edge::Hash>::iterator it, const Grid<T>& matrix);

        template <typename T>
        static std::pair<Identifiable, Identifiable> getNeighbours(const Edge& edge, const Grid<T>& matrix);

        template <typename T>
        static std::pair<Identifiable, Identifiable> getNeighbours(const Border::Segment &segment, const Grid<T> &grid);

        std::vector<Segment> segments;
        
        size_t minPassWidth = 0;
        size_t maxPassWidth = 0;
        size_t minWallLength = 0;
        size_t maxWallLength = 0;
        std::vector<std::vector<std::optional<size_t>>> nextPassIndex;
        // nextPassIndex[i][x] means furthest possible index j for index i to make pass between (i, j) of manhattan distance (pass width) x+minPassWidth.
        // i.e. nextPassIndex[i][0] is the furthest j which makes pass (i, j) of width minPassWidth.

        //dynamic programming calculations for pure random
        std::vector<size_t> wallStartingCombs;
        std::vector<size_t> passStartingCombs;
        size_t getAnyStartingCombs(size_t i) const{return wallStartingCombs.at(i) + passStartingCombs.at(i);};
    };

    template <typename T>
    std::unordered_map<std::pair<Identifiable, Identifiable>, std::vector<Border>, PairIDHash> Border::getAllBorders(const Grid<T>& grid){
        std::unordered_map<std::pair<Identifiable, Identifiable>, std::vector<Border>, PairIDHash> result;
        std::unordered_set<Border::Edge, Border::Edge::Hash> freeBorderSegments;

        for (size_t r = 0; r < grid.getHeight() - 1; r++){
            for (size_t c = 0; c < grid.getWidth(); c++){
                if (grid.getTileID(c, r) != grid.getTileID(c, r+1)){
                    freeBorderSegments.insert(Border::Edge({static_cast<int>(c), static_cast<int>(r)}, Orientation::vertical));
                }
            }
        }
        for (size_t r = 0; r < grid.getHeight(); r++){
            for (size_t c = 0; c < grid.getWidth() - 1; c++){
                if (grid.getTileID(c, r) != grid.getTileID(c+1, r)){
                    freeBorderSegments.insert(Border::Edge({static_cast<int>(c), static_cast<int>(r)},  Orientation::horizontal));
                }
            }
        }
        while (freeBorderSegments.size()){
            auto borderPair = getNeighbours(*(freeBorderSegments.begin()), grid);
            result[borderPair].push_back(Border(freeBorderSegments, freeBorderSegments.begin(), grid));
        }
        return result;
    }

    template <typename T>
    Border::Border(std::unordered_set<tiles::Border::Edge, Edge::Hash>& freeBorderEdges, std::unordered_set<tiles::Border::Edge, Edge::Hash>::iterator it, const Grid<T>& grid){
        freeBorderEdges.erase(it);
        Border::Segment middleSegment(*it);
        std::pair<Identifiable, Identifiable> neighbours = getNeighbours(middleSegment, grid);
        Border::Segment currentSegment = middleSegment;
        bool hasNext = true;
        
        segments.push_back(currentSegment);
        while (hasNext){
            hasNext = false;
            for (const Border::Segment& nextSegment : currentSegment.getNext()){
                auto nextEdgeIt = freeBorderEdges.find(Edge(nextSegment));
                if (nextEdgeIt == freeBorderEdges.end()){
                    continue;
                }
                auto compareNeighbours = getNeighbours(nextSegment, grid);
                if (neighbours != compareNeighbours){
                    continue;
                }
                freeBorderEdges.erase(nextEdgeIt);
                segments.push_back(nextSegment);
                currentSegment = nextSegment;
                hasNext = true;
                break;
            }
        }
        std::reverse(segments.begin(), segments.end());
        currentSegment = middleSegment;
        hasNext = true;
        while (hasNext){
            hasNext = false;
            for (const Border::Segment& nextSegment : currentSegment.getPrev()){
                auto nextEdgeIt = freeBorderEdges.find(Edge(nextSegment));
                if (nextEdgeIt == freeBorderEdges.end()){
                    continue;
                }
                auto compareNeighbours = getNeighbours(nextSegment, grid);
                if (neighbours != compareNeighbours){
                    continue;
                }
                freeBorderEdges.erase(nextEdgeIt);
                segments.push_back(nextSegment);
                currentSegment = nextSegment;
                hasNext = true;
                break;
            }
        }
        nextPassIndex = std::vector<std::vector<std::optional<size_t>>>(segments.size());
    }

    template <typename T>
    std::pair<Identifiable, Identifiable> Border::getNeighbours(const Border::Edge &edge, const Grid<T> &grid){
        return {grid.getTileID(edge.getTopLeftTile()), grid.getTileID(edge.getbottomRightTile())};
    }
    template <typename T>
    std::pair<Identifiable, Identifiable> Border::getNeighbours(const Border::Segment &segment, const Grid<T> &grid){
        return {grid.getTileID(segment.getLeftPos()), grid.getTileID(segment.getRightPos())};
    }
}