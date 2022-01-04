#include "metrics.hpp"

using namespace std;
using namespace cxxmetrics;

int main() {

    /**
     * tracy(My Laptop):
     *  start: 50.667 cycles (use __rdtsc)
     *  end: 78.406 cycles
     * 
     * handystats:
     *   200ns
     * 
     */

    const size_t T = 16*1024;
    const size_t N = 1024;

    auto st = TscClock::now();
    for(size_t i=0; i<T; i++)
        for(size_t j=0; j<N;j++)
            TscClock::now();
    auto ed = TscClock::now();
    cout << "Raw clock: " << (ed - st) / (double) (T * N) << endl;

    Metrics<TscClock, ArrayContainer<Event>> m(N * 2);
    uint64_t sum_st = 0;
    uint64_t sum_ed = 0;
    for(uint64_t i=0; i<T; i++) {
        st = TscClock::now();
        for(size_t j=0; j<N;j++)
            m.start_timer("test");
        ed = TscClock::now();
        sum_st += ed - st;
        st = TscClock::now();
        for(size_t j=0; j<N;j++)
            m.stop_timer();
        ed = TscClock::now();
        sum_ed += ed - st;
        m.queue_.clear();
    }
    
    cout << "Metrics Start: " << sum_st / (double) (T * N) << endl;
    cout << "Metrics Stop: " << sum_ed / (double) (T * N) << endl;
    printf("%.16f\n", TscClock::to_duration<chrono::duration<double, std::nano>>(st, ed).count() / N);
    return 0;
}