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

#include <asm/unistd.h>
#include <linux/perf_event.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cpuid.h>
#include <x86intrin.h>

// #ifdef __x86_64__
// #define CXXMETRICS_USE_TSC
// #endif

// #if defined(__clang__)

// #elif defined(__GNUC__) || defined(__GNUG__)

// #elif defined(_MSC_VER)



#if __cplusplus >= 202002L
#define CXXMETRICS_USE_CONCEPTS
#endif

#ifdef CXXMETRICS_USE_CONCEPTS
#include <concepts>
#endif

namespace cxxmetrics {

#ifdef CXXMETRICS_USE_CONCEPTS
    template <typename T>
    concept TICKER = requires {
        {T::now()} -> std::same_as<uint64_t>;
        {T::rate()} -> std::convertible_to<double>;
    };
#else
#define TICKER typename
#endif

    class TscClock
    {
    public:
        inline static uint64_t now() {
            // uint64_t rax, rdx;
            // std::atomic_signal_fence(std::memory_order_acq_rel);
            // asm volatile ( "rdtsc" : "=a" (rax), "=d" (rdx) );
            // std::atomic_signal_fence(std::memory_order_acq_rel);
            // return ( rdx << 32 ) | rax;
            return __rdtsc();
        }

        /**
         * @brief Estimate the TSC frequency.
         * 
         * @details Use high_resolution_clock to estimate the TSC frequency.
         * Note: NTP service may skew the system time, so this estimate may not be very precise,
         * even when use a very long sleep time.
         * 
         * @param sleep_time_ms The sleep time for estimate. 
         * @return uint64_t TSC frequency, in HZ.
         */
        inline static uint64_t estimate_tsc_frequency(uint64_t sleep_time_ms=200) {
            const auto t0 = std::chrono::high_resolution_clock::now();
            const auto r0 = now();
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_ms));
            const auto t1 = std::chrono::high_resolution_clock::now();
            const auto r1 = now();

            const auto dt = std::chrono::duration<double>(t1 - t0).count();
            const uint64_t dr = r1 - r0;

            return llround(dr / dt);
        }

        /**
         * @brief the clock rate
         * 
         * @param force_recompute force to recompute the rate, if false only compute in first time.
         * @return uint64_t Clock rate in HZ
         */
        inline static uint64_t rate(bool force_recompute=false, uint64_t sleep_time_ms=200) {
            static uint64_t r = 0;
            if(r == 0 || force_recompute) {
                auto p = get_tsc_mult_shift();
                if(p.has_value()) {
                    auto [mult, shift] = p.value();
                    double tsc_ghz = static_cast<double>(1ull << shift) / mult;
                    uint64_t tsc_khz = llround(tsc_ghz * 1e6);
                    r = tsc_khz * 1000;
                } else {

                    r = estimate_tsc_frequency(sleep_time_ms);
                }
            }
            return r;
        }

        template<typename ToDuration = std::chrono::duration<double>>
        inline static ToDuration to_duration(uint64_t st, uint64_t ed) {
            using Period = typename ToDuration::period;
            using Rep = typename ToDuration::rep;
            double seconds = (ed - st) / static_cast<double>(rate());
            Rep periods = static_cast<Rep>(seconds * Period::den / Period::num);
            return ToDuration(periods);
        }
    private:

        // from https://stackoverflow.com/questions/35123379/getting-tsc-rate-from-x86-kernel
        inline static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                int cpu, int group_fd, unsigned long flags)
        {
            return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
        }

        inline static std::optional<std::pair<int64_t, int64_t>> get_tsc_mult_shift() {
            struct perf_event_attr pe = {
            .type = PERF_TYPE_HARDWARE,
            .size = sizeof(struct perf_event_attr),
            .config = PERF_COUNT_HW_INSTRUCTIONS,
            .disabled = 1,
            .exclude_kernel = 1,
            .exclude_hv = 1
            };
            int fd = perf_event_open(&pe, 0, -1, -1, 0);
            if (fd == -1) {
                return {};
            }
            void *addr = mmap(NULL, 4*1024, PROT_READ, MAP_SHARED, fd, 0);
            if (!addr) {
                return {};
            }
            perf_event_mmap_page *pc = (perf_event_mmap_page*)addr;
            if (pc->cap_user_time != 1) {
                return {};
            }
            close(fd);
            return std::make_pair(pc->time_mult, pc->time_shift);
        }
    };

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