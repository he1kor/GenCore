#pragma once

class Identifiable{
    protected:
        int id;
    public:
        virtual ~Identifiable() = default;
        int getID();
};