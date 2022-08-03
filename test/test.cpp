#include <iostream>
#include <thread>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "cxxmetrics/ticker.h"

template<TICKER Ticker>
double sync_rate(uint64_t sleep_ms) {
    auto std_st = std::chrono::high_resolution_clock::now();
    uint64_t st = Ticker::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    auto std_ed = std::chrono::high_resolution_clock::now();
    uint64_t ed = Ticker::now();

    auto std_time = std::chrono::duration<double>(std_ed - std_st).count();
    double freq = Ticker::rate();
    double ticker_time = (ed - st) / freq;
    
    return ticker_time / std_time;
}


TEST_CASE("Tickers") {
    double sync = sync_rate<DefaultTicker>(200);
    CHECK(sync < 1.0001);
    CHECK(sync > 0.9999);

    sync = sync_rate<TscTicker>(200);
    CHECK(sync < 1.0001);
    CHECK(sync > 0.9999);

    sync = sync_rate<StdTicker<std::chrono::system_clock>>(200);
    CHECK(sync < 1.0001);
    CHECK(sync > 0.9999);

    sync = sync_rate<StdTicker<std::chrono::steady_clock>>(200);
    CHECK(sync < 1.0001);
    CHECK(sync > 0.9999);

    sync = sync_rate<StdTicker<std::chrono::high_resolution_clock>>(200);
    CHECK(sync < 1.0001);
    CHECK(sync > 0.9999);

}

