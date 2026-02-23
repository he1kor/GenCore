#pragma once
#include <functional>

#include <iostream>



class Identifiable{
    protected:
        int id = nullID;
    public:
        static inline const int nullID = -1;
        Identifiable(int id);
        Identifiable() : id(nullID){};
        virtual ~Identifiable() = default;
        int getID() const;
        bool operator==(const Identifiable& other) const;
        bool operator>(const Identifiable& other) const;
        bool operator<(const Identifiable& other) const;
        bool operator>=(const Identifiable& other) const;
        bool operator<=(const Identifiable& other) const;
        
        bool operator!=(const Identifiable& other) const;
        std::string toString() const;
};

template<typename T>
concept hasID = std::is_base_of_v<Identifiable, T>;


std::ostream& operator<<(std::ostream& os, const Identifiable& id);

struct IDHash{
    std::size_t operator()(const Identifiable& e) const {
        return std::hash<int>{}(e.getID());
    }
};

struct PairIDHash {
    std::size_t operator()(const std::pair<Identifiable, Identifiable>& e) const {
        auto [a, b] = PairIDHash::normalize(e);
        return std::hash<int>{}(a.getID()) ^ (std::hash<int>{}(b.getID()) << 1);
    }

    static std::pair<Identifiable, Identifiable> normalize(const std::pair<Identifiable, Identifiable>& pair){
        return std::make_pair(
            Identifiable(std::min(pair.first.getID(), pair.second.getID())),
            Identifiable(std::max(pair.first.getID(), pair.second.getID()))
        );
    }
};

struct AsymPairIDHash {
    std::size_t operator()(const std::pair<Identifiable, Identifiable>& e) const {
        return std::hash<int>{}(e.first.getID()) ^ (std::hash<int>{}(e.second.getID()) << 1);
    }
};