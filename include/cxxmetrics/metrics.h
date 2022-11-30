#ifndef __CXXMETRICS__HPP__
#define __CXXMETRICS__HPP__

#include <any>
#include <atomic>
#include <chrono>
#include <exception>
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

class MetricsMeter {
public:
    virtual std::string ToString() {
        return "(EmptyMeter)";
    }
    virtual ~MetricsMeter() {};
};

template<TICKER Ticker=DefaultTicker>
class Timer : public MetricsMeter {
public:

    Timer() : st_(0), ed_(0) {}

    ~Timer() override {}

    void Start() {
        st_ = Ticker::now();
    }

    int64_t Stop() {
        ed_ = Ticker::now();
        return ed_ - st_;
    }

    double GetTime() {
        if(ed_ != 0) {
            return ToDuration<Ticker>(ed_ - st_).count();
        } else {
            return 0;
        }
    }

private:
    uint64_t st_;
    uint64_t ed_; 
};

using DefaultTimer = Timer<>;

template<typename T> 
class Attribute : public MetricsMeter {

    Attribute() = default;

    Attribute(const T& val) {
        data_ = val;
    }

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
    Metrics(size_t buffer_size=1024*1024) : pool_(buffer_size), allocator_(&pool_) {
    }

    template<typename T>
    void Set(std::string_view key, T value) {
        TryNewEntry<Attribute<T>>(key, value);
    }

    template<typename T>
    T Get(std::string_view key, T value) {
        return GetEntry<Attribute<T>>(key)->Get();
    }

    void StartTimer(std::string_view key) {
        TryNewEntry<DefaultTimer>(key)->Start();
    }

    int64_t StopTimer(std::string_view key) {
        return GetEntry<DefaultTimer>(key)->Stop();
    }

private:
    template<typename T, typename... Args>
    T* TryNewEntry(std::string_view key, const Args&... args) {
        auto iter = map_.find(key);
        if(iter != std::end(map_)) {
            return dynamic_cast<T*>(iter->second);
        } else {
            T* entry = allocator_.new_object<DefaultTimer>();
            map_[key] = entry;
            return entry;
        }
    }

    template<typename T>
    T* GetEntry(std::string_view key) {
        return dynamic_cast<DefaultTimer*>(map_[key]);
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

inline int64_t StopTimer(std::string_view key) {
    return GlobalMetrics().StopTimer(key);
}

template<typename T>
inline void SetEntry(std::string_view key, T value) {
    GlobalMetrics().Set(value);
}

}

#endif