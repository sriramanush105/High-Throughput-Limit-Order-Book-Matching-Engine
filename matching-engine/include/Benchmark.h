// Benchmark.h
// -----------------------------------------------------------------------------
// Synthetic load generator and microbenchmark. Produces orders around a
// configurable mid-price using exponentially-distributed inter-arrival times
// (Poisson process) and measures nanoseconds-per-operation.
// -----------------------------------------------------------------------------

#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "OrderBook.h"
#include <cstdint>

struct BenchmarkResult {
    uint64_t operations     = 0;
    double   elapsed_seconds = 0.0;
    double   ns_per_op       = 0.0;
    double   ops_per_second  = 0.0;
    uint64_t trades_emitted  = 0;
};

class Benchmark {
public:
    // Run `num_ops` synthetic orders against `book`. Mid price is given in
    // LOBSTER integer format (e.g. 310000 = $31.00). `cancel_prob` is the
    // fraction of operations that are cancellations instead of new orders.
    static BenchmarkResult run(OrderBook& book,
                               uint64_t num_ops,
                               int64_t mid_price = 310000,
                               double  cancel_prob = 0.30,
                               uint32_t seed = 42);
};

#endif // BENCHMARK_H
