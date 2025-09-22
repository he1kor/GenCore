#include "border.h"

#include <stdexcept>
#include <format>

#include "random_generator.h"
#include <iomanip>

using namespace tiles;

Border::Edge::Edge(IntVector2 aTile, IntVector2 bTile){
    if (aTile.x == bTile.x){
        if (abs(aTile.y - bTile.y) != 1){
            throw std::invalid_argument("Given tiles are not 4-connected neighbours");
        }
        orientation = Orientation::vertical;
        topLeftPos = {aTile.x, std::min(aTile.y, bTile.y)};
    }
    else if (aTile.y == bTile.y){
        if (abs(aTile.x - bTile.x) != 1){
            throw std::invalid_argument("Given tiles are not 4-connected neighbours");
        }
        orientation = Orientation::horizontal;
        topLeftPos = {std::min(aTile.x, bTile.x), aTile.y};
    }
    else {
        throw std::invalid_argument("Given tiles are not 4-connected neighbours");
    }
}

Border::Edge::Edge(const Segment& segment){
    orientation =
        segment.direction == Direction::right || segment.direction == Direction::left ? Orientation::vertical : Orientation::horizontal;

    this->topLeftPos =
        segment.direction == Direction::right || segment.direction == Direction::up ? segment.getLeftPos() : segment.getRightPos();
}

IntVector2 Border::Edge::getTopLeftTile() const{
    return topLeftPos;
}

IntVector2 Border::Edge::getbottomRightTile() const{
    IntVector2 result = topLeftPos;
    switch (orientation){
        case Orientation::horizontal:
            result.x++;
            break;
        case Orientation::vertical:
            result.y++;
            break;
    }
    return result;
}

Border::Segment::Segment(IntVector2 aTile, IntVector2 bTile){
    if (aTile.x == bTile.x){
        int dif = bTile.y - aTile.y;
        if (dif == 1){
            direction == Direction::down;
        } else if (dif == -1){
            direction == Direction::up;
        } else{
            throw std::invalid_argument("Given tiles are not 4-connected neighbours");
        }
    }
    else if (aTile.y == bTile.y){
        int dif = bTile.x - aTile.x;
        if (dif == 1){
            direction == Direction::right;
        } else if (dif == -1){
            direction == Direction::left;
        } else{
            throw std::invalid_argument("Given tiles are not 4-connected neighbours");
        }
    }
    else {
        throw std::invalid_argument("Given tiles are not 4-connected neighbours");
    }

    startPos = aTile;
}

tiles::Border::Segment::Segment(const Edge &edge){
    if (edge.orientation == Orientation::horizontal){
        direction = Direction::down;
    } else{
        direction = Direction::right;
    }
    startPos = edge.getbottomRightTile();
}

void Border::Segment::turnLeft(){
    ::turnLeft(direction);
}

void Border::Segment::turnRight(){
    ::turnRight(direction);
}

void tiles::Border::Segment::turnBackward(){
    ::turnBackward(direction);
}

IntVector2 tiles::Border::Segment::getLeftPos() const{
    IntVector2 result = startPos;
    switch (direction){
        case Direction::up:
            result.x--;
            result.y--;
            break;
        case Direction::left:
            result.x--;
            break;
        case Direction::right:
            result.y--;
            break;
        case Direction::down:
            break;
    }
    return result;
}

IntVector2 tiles::Border::Segment::getRightPos() const{
    IntVector2 result = startPos;
    switch (direction){
        case Direction::right:
            break;
        case Direction::down:
            result.x--;
            break;
        case Direction::up:
            result.y--;
            break;
        case Direction::left:
            result.x--;
            result.y--;
            break;
    }
    return result;
}

void Border::Segment::moveForward(){
    setPos(getNextPos());
}

bool Border::Segment::fits(Edge &edge) const{
    if (edge.orientation != toOrientation(direction))
        return false;
    const IntVector2& topLeftPos =
        direction == Direction::right || direction == Direction::down ? startPos : getNextPos();
    return topLeftPos == edge.topLeftPos;
}

std::array<Border::Segment, 6> Border::Segment::getNeighbours() const{
    Border::Segment segment = *this;
    segment.moveForward();
    std::array<Border::Segment, 6> result ={
        *this,
        *this,
        *this,
        segment,
        segment,
        segment
    };
    result[0].turnLeft();
    result[1].turnBackward();
    result[2].turnRight();
    
    result[3].turnLeft();
    result[5].turnRight();
    return result;
}

std::array<Border::Segment, 3> Border::Segment::getNext() const{
    Border::Segment segment = *this;
    segment.moveForward();
    std::array<Border::Segment, 3> result;
    result.fill(segment);
    result[0].turnLeft();
    result[2].turnRight();
    return result;
}

std::array<Border::Segment, 3> Border::Segment::getPrev() const{
    std::array<Border::Segment, 3> result;
    result.fill(*this);

    result[0].turnLeft();
    result[0].moveForward();

    result[2].turnRight();
    result[2].moveForward();
    
    result[1].turnBackward();
    result[1].moveForward();

    result[0].turnBackward();
    result[1].turnBackward();
    result[2].turnBackward();
    return result;
}

void Border::Segment::setPos(IntVector2 pos){
    this->startPos = pos;
}

void Border::Segment::setDirection(Direction direction){
    this->direction = direction;
}

size_t tiles::Border::manhattanFromTo(size_t i1, size_t i2) const{
    int xDif = segments.at(i1).getNextPos().x - segments.at(i2).getStartPos().x;
    int yDif = segments.at(i1).getNextPos().y - segments.at(i2).getStartPos().y;
    return std::abs(xDif) + std::abs(yDif);
}

IntVector2 Border::Segment::getNextPos() const{
    IntVector2 move;
    switch (direction){
        case Direction::up:
            move = {0, -1};
            break;
        case Direction::left:
            move = {-1, 0};
            break;
        case Direction::right:
            move = {1, 0};
            break;
        case Direction::down:
            move = {0, 1};
            break;
    }
    return startPos + move;
}

void tiles::Border::initPassData(PassParams params){
    this->params = params;

    if (params.maxPassWidth < params.minPassWidth){
        throw std::invalid_argument(std::format("maxPassWidth ({}) is greater than minPassWidth ({})", params.maxPassWidth, params.minPassWidth));
    }
    size_t numberPasses = params.maxPassWidth - params.minPassWidth + 1;

    nextPassIndex.assign(segments.size(), std::vector<std::optional<size_t>>());

    std::vector<size_t> currentFurthestIndex(numberPasses, 0);
    for (size_t i = 0; i < segments.size(); i++){
        nextPassIndex[i] = std::vector<std::optional<size_t>>(numberPasses, std::nullopt);
        if (i == segments.size() - 1){
            continue;
        }
        for (size_t j = i+1; j < segments.size() - 1; j++){
            size_t distance = manhattanFromTo(i, j);
            if (distance < params.minPassWidth || distance > params.maxPassWidth) continue;
            size_t widthIndex = distance - params.minPassWidth;
            if (distance <= params.maxPassWidth && distance >= params.minPassWidth && j > currentFurthestIndex[widthIndex]){
                nextPassIndex[i][widthIndex] = j;
                currentFurthestIndex[widthIndex] = j;
            }
        }
    }
    initCombinations();
}

void tiles::Border::initCombinations(){
    const size_t size = segments.size();
    size_t numberPasses = params.maxPassWidth - params.minPassWidth + 1;
    wallStartingCombs.assign(size, 0);
    passStartingCombs.assign(size, 0);

    size_t n = std::min(size, params.minWallLength);
    std::fill(wallStartingCombs.end() - n, wallStartingCombs.end(), 1);
    passStartingCombs.back() = 1;

    for (size_t i = size - 1; i < size; i--){
        for (size_t wall_r = i == 0 ? 0 : i+params.minWallLength; wall_r < size && wall_r <= i+params.maxWallLength; wall_r++){
            wallStartingCombs[i] += passStartingCombs[wall_r];
        }
        for (auto pass_i = 0; pass_i < numberPasses; pass_i++){
            if (!nextPassIndex[i][pass_i]) continue;
            passStartingCombs[i] += wallStartingCombs[*nextPassIndex[i][pass_i]];
        }
    }
}

void tiles::Border::logNextPassIndex() {
    std::clog << "nextPassIndex (by columns):\n";
    
    if (nextPassIndex.empty() || nextPassIndex[0].empty()) {
        std::clog << "[Empty]\n" << std::endl;
        return;
    }
    
    size_t rows = nextPassIndex.size();
    size_t cols = nextPassIndex[0].size();
    
    for (size_t j = 0; j < cols; ++j) {
        std::clog << "[";
        
        for (size_t i = 0; i < rows; ++i) {
            if (nextPassIndex[i][j].has_value()) {
                std::clog << std::setw(3) << nextPassIndex[i][j].value();
            } else {
                std::clog << std::setw(3) << "---";
            }
            
            if (i < rows - 1) {
                std::clog << " ";
            }
        }
        
        std::clog << "]\n";
    }
    
    std::clog << std::endl;
}

void tiles::Border::logStartingCombs() {
    std::clog << "Wall Starting Combinations:\n";
    
    if (wallStartingCombs.empty()) {
        std::clog << "[Empty]\n";
    } else {
        std::clog << "[";
        for (size_t i = 0; i < wallStartingCombs.size(); ++i) {
            std::clog << std::setw(3) << wallStartingCombs[i];
            if (i < wallStartingCombs.size() - 1) {
                std::clog << " ";
            }
        }
        std::clog << "]\n";
    }
    
    std::clog << "\nPass Starting Combinations:\n";
    
    if (passStartingCombs.empty()) {
        std::clog << "[Empty]\n";
    } else {
        std::clog << "[";
        for (size_t i = 0; i < passStartingCombs.size(); ++i) {
            std::clog << std::setw(3) << passStartingCombs[i];
            if (i < passStartingCombs.size() - 1) {
                std::clog << " ";
            }
        }
        std::clog << "]\n";
    }
    
    std::clog << std::endl;
}

void tiles::Border::logPassData(){
    logStartingCombs();
    logNextPassIndex();
}

const Border::Segment &Border::at(size_t i) const{
    return segments.at(i);
}

std::vector<Border::PossiblePass> tiles::Border::possiblePasses(size_t index) const{
    std::vector<Border::PossiblePass> result;
    for (size_t i = 0; i <= params.maxPassWidth - params.minPassWidth; i++){
        if (const std::optional<size_t>& j = nextPassIndex.at(index)[i]){
            result.emplace_back(index, *j, i+params.minPassWidth);
        }
    }
    return result;
}

Border::PossiblePass tiles::Border::generatePass(size_t i){
    std::vector<uint64_t> widthWeights;
    for (size_t j = 0; j <= params.maxPassWidth - params.minPassWidth; j++){
        if (const std::optional<size_t>& optionalNextEnd = nextPassIndex.at(i)[j]){
            widthWeights.push_back(wallStartingCombs[*optionalNextEnd]);
        } else{
            widthWeights.push_back(0);
        }
    }
    size_t chosenWidth_i = RandomGenerator::instance().takenIndex(widthWeights);
    
    PossiblePass result{
        .firstIndex = i,
        .secondIndex = *nextPassIndex[i].at(chosenWidth_i),
        .width = i + params.minPassWidth
    };
    
    for (size_t j = result.firstIndex; j < result.secondIndex; j++){
        disable(j);
    }

    return result;
}

size_t tiles::Border::generateWall(size_t i){
    std::vector<uint64_t> lengthWeights;
    for (size_t right_i = i == 0 ? 0 : i + params.minWallLength; right_i < segments.size() && right_i <= i + params.maxWallLength; right_i++){
        if (passStartingCombs.at(right_i) == 0) continue;
        lengthWeights.push_back(passStartingCombs.at(right_i));
    }
    if (i == 0) return i + RandomGenerator::instance().takenIndex(lengthWeights);
    return i + params.minWallLength + RandomGenerator::instance().takenIndex(lengthWeights);
}

void tiles::Border::generatePasses(){
    size_t i = 0;
    size_t size = segments.size();

    bool isWallTurn = RandomGenerator::instance().takenIndex({wallStartingCombs[0], passStartingCombs[0]}) == 0;

    while (i < size - 1){
        if (isWallTurn){
            while (i < size - 1 && wallStartingCombs[i] == 0){
                i++;
            }
            if (i >= size - 1) break;
            i = generateWall(i);
        } else {
            while (i < size - 1 && passStartingCombs[i] == 0){
                i++;
            }
            if (i >= size - 1) break;
            i = generatePass(i).secondIndex;
        }
        isWallTurn = !isWallTurn;
    }
}

IntVector2 tiles::Border::getLeft(size_t i){
    return segments[i].getLeftPos();
}

IntVector2 tiles::Border::getRight(size_t i){
    return segments[i].getRightPos();
}

std::size_t tiles::Border::Edge::Hash::operator()(const Edge& segment) const{
    std::hash<int> int_hasher;
    std::size_t pos_hash = int_hasher(segment.topLeftPos.x);
    pos_hash ^= int_hasher(segment.topLeftPos.y) + 0x9e3779b9 + (pos_hash << 6) + (pos_hash >> 2);
    
    std::size_t dir_hash = static_cast<std::size_t>(segment.orientation);
    
    return pos_hash ^ (dir_hash + 0x9e3779b9 + (pos_hash << 6) + (pos_hash >> 2));
}


std::size_t tiles::Border::Segment::Hash::operator()(const Segment& segment) const{
    std::hash<int> int_hasher;
    std::size_t pos_hash = int_hasher(segment.startPos.x);
    pos_hash ^= int_hasher(segment.startPos.y) + 0x9e3779b9 + (pos_hash << 6) + (pos_hash >> 2);
    
    std::size_t dir_hash = static_cast<std::size_t>(segment.direction);
    
    return pos_hash ^ (dir_hash + 0x9e3779b9 + (pos_hash << 6) + (pos_hash >> 2));
}
