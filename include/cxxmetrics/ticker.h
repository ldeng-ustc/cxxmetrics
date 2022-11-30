/**
 * @file ticker.h
 * @author Long Deng (ldeng@mail.ustc.edu.cn)
 * @brief Ticker class, a swift, dynamic clock class
 * @version 0.1
 * @date 2022-02-25
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef __CXXMETRICS_ticker__HPP__
#define __CXXMETRICS_ticker__HPP__

#include <chrono>
#include <optional>
#include <thread>
#include <cmath>
#include "macro.h"

#ifdef CXXMETRICS_USE_CONCEPTS
#include <concepts>
#endif

#ifdef CXXMETRICS_LINUX
#include <sys/mman.h>
#include <linux/perf_event.h>
#include <unistd.h>
#endif

#ifdef CXXMETRICS_USE_TSC
#if defined(CXXMETRICS_GCC) || defined(CXXMETRICS_CLANG)
#include <x86intrin.h>
#elif defined(CXXMETRICS_VS)
#include <intrin.h>
#endif
#endif

#ifdef CXXMETRICS_USE_CONCEPTS
template <typename T>
concept TICKER = requires {
    {T::now()} -> std::same_as<uint64_t>;
    {T::rate()} -> std::convertible_to<double>; // maybe freq()?
};

template <typename T>
concept STDCLOCK = std::chrono::is_clock<T>::value;
#else
#define TICKER    typename
#define STDCLOCK  typename
#endif

#ifdef CXXMETRICS_USE_TSC
class TscTicker
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

private:

#ifdef __linux__
    // from https://stackoverflow.com/questions/35123379/getting-tsc-rate-from-x86-kernel
    inline static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
            int cpu, int group_fd, unsigned long flags) {
#ifdef __NR_perf_event_open
        return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
#else
        return -1;
#endif
    }
#endif

    inline static std::optional<std::pair<int64_t, int64_t>> get_tsc_mult_shift() {
#ifdef __linux__
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
#else
        return {};
#endif
    }
};
#endif

template <STDCLOCK Clock=std::chrono::high_resolution_clock>
class StdTicker {
public:
    inline static uint64_t now() {
        static auto start = Clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
    }

    constexpr static uint64_t rate() {
        return std::nano::den;
    }
};

template<TICKER Ticker, typename TargetDuration = std::chrono::duration<double>>
inline static TargetDuration ToDuration(int64_t dur) {
    using Period = typename TargetDuration::period;
    using Rep = typename TargetDuration::rep;
    double seconds = dur / static_cast<double>(Ticker::rate());
    Rep periods = static_cast<Rep>(seconds * Period::den / Period::num);
    return TargetDuration(periods);
}


#ifdef CXXMETRICS_USE_TSC
using DefaultTicker = TscTicker;
#else
using DefaultTicker = StdTicker<>;
#endif

#endif