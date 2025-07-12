#pragma once
#include <functional>


class Identifiable{
    protected:
        int id = -1;
    public:
        Identifiable(int id);
        Identifiable() = default;
        virtual ~Identifiable() = default;
        int getID() const;
        bool operator==(const Identifiable& other) const;
        bool operator>(const Identifiable& other) const;
        bool operator<(const Identifiable& other) const;
        bool operator>=(const Identifiable& other) const;
        bool operator<=(const Identifiable& other) const;
        
        bool operator!=(const Identifiable& other) const;
};

struct IDHash{
    std::size_t operator()(const Identifiable& e) const {
        return std::hash<int>{}(e.getID());
    }
};

struct PairIDHash {
    std::size_t operator()(const std::pair<Identifiable, Identifiable>& e) const {
        auto a = std::min(e.first.getID(), e.second.getID());
        auto b = std::max(e.first.getID(), e.second.getID());
        return std::hash<int>{}(a) ^ (std::hash<int>{}(b) << 1);
    }
};