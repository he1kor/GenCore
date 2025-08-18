#pragma once

#include <optional>

template<typename T>
class SelfPointerOwner;

template<typename T>
class SelfPointer{
    public:
        SelfPointer(){}

        SelfPointer(const SelfPointer& other) = default;
        SelfPointer& operator=(const SelfPointer& other) = default;

        SelfPointer(std::nullptr_t){};
        SelfPointer& operator=(std::nullptr_t){
            this = SelfPointer(nullptr);
            return *this;
        };

        SelfPointer(SelfPointer&& other) noexcept = default;
        SelfPointer& operator=(SelfPointer&& other) noexcept = default;

        bool operator==(std::nullptr_t) const{
            return !isValid();
        }
        bool operator!=(std::nullptr_t) const{
            return isValid();
        }
        bool operator==(const SelfPointer& other) const {
            return ptr == other.ptr;
        }

        bool operator!=(const SelfPointer& other) const {
            return ptr != other.ptr;
        }

        ~SelfPointer() = default;
        bool isValid() const{
            return available && *available;
        }
        explicit operator bool() const { return isValid(); }
        std::optional<std::reference_wrapper<T>> lock() const{
            if (isValid())
                return std::ref(*ptr);
            return std::nullopt;
        }
    private:
        friend class SelfPointerOwner<T>;
        explicit SelfPointer(T* ptr, std::shared_ptr<bool> available) : ptr{ptr}, available{std::move(available)}{}
        T* ptr;
        std::shared_ptr<bool> available;
};

template<typename T>
class SelfPointerOwner{
    
    private:
        std::shared_ptr<bool> available;

    public:
        SelfPointerOwner() : available(std::make_shared<bool>(true)){
            static_assert(std::derived_from<T, SelfPointerOwner<T>>,
                "T must inherit from SelfPointerOwner<T>");
        }
        virtual ~SelfPointerOwner(){
            *available = false;
        }
        SelfPointer<T> self(){
            return SelfPointer<T>(static_cast<T*>(this), available);
        }
    protected:
        SelfPointerOwner(const SelfPointerOwner& other) : available(std::make_shared<bool>(true)){}
        SelfPointerOwner& operator=(const SelfPointerOwner& other){
            available = std::make_shared<bool>(true);
            return *this;
        }

        SelfPointerOwner(SelfPointerOwner&& other) {
            available = std::move(other.available);
            other.available = std::make_shared<bool>(false);
            *available = true;
        }
        SelfPointerOwner& operator=(SelfPointerOwner&& other) {
            if (this == &other)
                return *this;
            available = std::move(other.available);
            other.available = std::make_shared<bool>(false);
            *available = true;
            return *this;
        }
};