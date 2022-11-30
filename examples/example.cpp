#include "fmt/format.h"
#include "cxxmetrics/metrics.h"

using namespace std;
using namespace cxxmetrics;
using fmt::format;

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

    auto st = TscTicker::now();
    for(size_t i=0; i<T; i++)
        for(size_t j=0; j<N;j++)
            TscTicker::now();
    auto ed = TscTicker::now();
    cout << "Raw clock: " << (ed - st) / (double) (T * N) << endl;

    Metrics m;
    uint64_t sum_st = 0;
    uint64_t sum_ed = 0;
    for(uint64_t i=0; i<T; i++) {
        st = TscTicker::now();
        for(size_t j=0; j<N;j++)
            m.StartTimer("test");
        ed = TscTicker::now();
        sum_st += ed - st;
        st = TscTicker::now();
        for(size_t j=0; j<N;j++)
            m.StopTimer("test");
        ed = TscTicker::now();
        sum_ed += ed - st;
    }

    cout << "Metrics Start: " << sum_st / (double) (T * N) << endl;
    cout << "Metrics Stop: " << sum_ed / (double) (T * N) << endl;
    printf("%.16f\n", ToDuration<DefaultTicker>(ed - st).count());
    return 0;
}