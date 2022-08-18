#ifndef __CXXMETRICS__HPP__
#define __CXXMETRICS__HPP__

#include <any>
#include <atomic>
#include <chrono>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <memory_resource>
#include <optional>
#include <stack>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <cassert>
#include <cmath>
#include "ticker.h"

namespace cxxmetrics {

template<typename T>
struct Duration {
    int64_t cycles;
    Duration(int64_t cycles): cycles(cycles) {};
    
    template<typename StdDuration = std::chrono::duration<double>>
    StdDuration ToStdDuration() const {
        return T::to_duration(cycles);
    }
};

class MetricsMeter {
    virtual ~MetricsMeter();
};

template<TICKER Ticker=DefaultTicker>
class Timer : public MetricsMeter {
public:

    Timer() : tmp_(Ticker::now()) {}

    ~Timer() override {}

    void Start() {
        tmp_ = Ticker::now();
    }

    Duration<Ticker> Stop() {
        return tmp_ = Ticker::now() - tmp_;
    }

private:
    uint64_t tmp_; 
};

using DefaultTimer = Timer<>;

template<typename T> 
class Attribute : public MetricsMeter {
    void Set(T&& value) {
        data_ = value;
    }

    T& Get() {
        return data_;
    }
private:
    T data_;
};

class Metrics {
public:
    Metrics(size_t buffer_size=1024*1024) : pool_(buffer_size), allocator_(&pool_) {}

    template<typename T>
    void Set(std::string_view key, T value) {
        map_.emplace(key, New<Attribute<T>>());
    }

    template<typename T>
    T Get(std::string_view key, T value) {
        return dynamic_cast<Attribute<T>>(map_[key]).Get();
    }

    void StartTimer(std::string_view key) {
        map_.emplace(key, New<DefaultTimer>());
    }

    void StopTimer(std::string_view key) {
        dynamic_cast<Attribute<T>>(map_[key])
    }

private:
    template<typename T>
    T* New() {
        return allocator_.new_object<T>(value);
    }

    template<typename T>
    T* Get() {
        
    }

private:
    std::pmr::monotonic_buffer_resource pool_;
    std::pmr::polymorphic_allocator<> allocator_;
    std::unordered_map<std::string_view, MetricsMeter*> map_; 
};

inline auto& GlobalMetrics() {
    static Metrics m;
    return m;
}

inline void StartTimer(std::string_view key) {
    GlobalMetrics().StartTimer(key);
}

inline Duration<DefaultTicker> StopTimer(std::string_view key) {
    return GlobalMetrics().StopTimer(key);
}

template<typename T>
inline void SetEntry(std::string_view key, T value) {
    GlobalMetrics().Set(value);
}

}

#endif