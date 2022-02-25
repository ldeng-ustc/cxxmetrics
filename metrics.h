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

#ifdef CXXMETRICS_USE_CONCEPTS
    static_assert(TICKER<TscClock>);
#endif

    enum EventType{
        START_TIMER,
        STOP_TIMER
    };

    struct Event {
        EventType op;
        uint64_t ts;
        std::string_view name;
        std::any data;
    };

    struct RunningTimer {
        std::string_view name;
        uint64_t ts_start;
    };

    template<typename T>
    class ArrayContainer
    {
    private:
        std::unique_ptr<T[]> q_;
        size_t cap_;
        size_t n_;
    public:
        ArrayContainer(): q_(nullptr), cap_(0), n_(0) { }

        T* begin() {
            return q_.get();
        }

        T* end() {
            return q_.get() + n_;
        }

        void reserve(size_t cap) {
            q_ = std::make_unique<T[]>(cap);
            cap_ = cap;
        }

        void push_back(const T& item) {
            if(n_ == cap_) {
                throw std::overflow_error("ArrayContainer overflow");
            }
            q_[n_++] = item;
        }

        size_t size() {
            return n_;
        }

        void clear() {
            n_ = 0;
        }
    };

    template<TICKER Clock=TscClock, typename EventQueue=std::vector<Event>>
    struct Metrics
    {
    public:
        Metrics(size_t queue_size=1024*1024) {
            timer_count_ = 0;
            queue_.reserve(queue_size);
        }

        size_t start_timer(std::string_view name) {
            timer_count_ ++;
            auto cnt = queue_.size();
            queue_.push_back({EventType::START_TIMER, Clock::now(), name}); 
            return cnt;
        }

        void stop_timer() {
            if(timer_count_ == 0) {
                throw std::underflow_error("No timer can be stop.");
            }
            timer_count_ --;
            queue_.push_back({EventType::STOP_TIMER, Clock::now()});
            return ;
        }

        void collect() {
            for(const Event& e: queue_) {
                switch (e.op) {
                case EventType::START_TIMER: {
                    auto it = tlist_.insert(tlist_.end(), {e.name, e.ts});
                    tmap_.emplace(e.name, it);
                    break;
                }
                case EventType::STOP_TIMER: {
                    auto name = e.name;
                    if(name.empty()) {
                        name = tlist_.back().name;
                    }
                    auto map_it = --tmap_.upper_bound(name);
                    auto list_it = map_it->second;
                    assert(map_it->first == name);
                    assert(list_it->name == name);
                    auto it = map_.try_emplace(name).first;
                    it->second.push_back(e.ts - list_it->ts_start);
                    tlist_.erase(list_it);
                    tmap_.erase(map_it);
                    break;
                }
                default:
                    break;
                }
            }
            queue_.clear();
        }

        template<typename T> set(std::string_view name) {
        }

    //private:
    public:
        EventQueue queue_;
        uint64_t timer_count_;
        std::unordered_map<std::string_view, std::vector<uint64_t>> map_;
        std::list<RunningTimer> tlist_;
        std::multimap<std::string_view, decltype(Metrics::tlist_)::iterator> tmap_;
        
    };

    inline auto& global_metrics() {
        static Metrics m;
        return m;
    }

    inline size_t start_timer(std::string_view name) {
        global_metrics().start_timer(name);
    }

    inline void stop_timer() {
        global_metrics().stop_timer();
    }

}

#endif