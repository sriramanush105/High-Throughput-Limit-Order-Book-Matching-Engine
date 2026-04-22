// Benchmark.cpp
// -----------------------------------------------------------------------------

#include "Benchmark.h"
#include <random>
#include <chrono>
#include <vector>
#include <algorithm>

BenchmarkResult Benchmark::run(OrderBook& book,
                               uint64_t num_ops,
                               int64_t mid_price,
                               double  cancel_prob,
                               uint32_t seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> u01(0.0, 1.0);

    // Price offset in ticks around the mid. A "tick" here is $0.01 = 100 in
    // LOBSTER integer format. Normal distribution gives a realistic bell
    // curve of order prices clustered near the touch.
    std::normal_distribution<double> price_offset(0.0, 5.0);    // ~5-tick spread
    std::uniform_int_distribution<uint32_t> qty_dist(1, 500);
    std::exponential_distribution<double> interarrival(1000.0); // ~1ms mean

    std::vector<uint64_t> live_ids;
    live_ids.reserve(num_ops / 2);

    uint64_t next_id = 1;
    double   t       = 34200.0;   // 9:30 AM (seconds past midnight)

    auto start = std::chrono::high_resolution_clock::now();
    size_t trades_before = book.trades().size();

    for (uint64_t i = 0; i < num_ops; ++i) {
        t += interarrival(rng);
        double action = u01(rng);

        if (action < cancel_prob && !live_ids.empty()) {
            // Cancel a random live order.
            size_t idx = rng() % live_ids.size();
            uint64_t id = live_ids[idx];
            // Swap-remove to keep O(1) removal from the tracking vector.
            live_ids[idx] = live_ids.back();
            live_ids.pop_back();
            book.cancel_order(id);
        } else {
            // Submit a new order — skewed slightly to cross occasionally.
            bool is_buy = (rng() & 1u) == 0;
            int64_t tick_off = static_cast<int64_t>(price_offset(rng));
            int64_t px = mid_price + tick_off * 100
                                   + (is_buy ? -100 : 100);   // bias to rest
            if (px <= 0) px = mid_price;
            uint32_t q = qty_dist(rng);

            book.add_order(next_id, is_buy ? Side::BUY : Side::SELL, px, q, t);
            live_ids.push_back(next_id);
            ++next_id;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> dt = end - start;

    BenchmarkResult r;
    r.operations      = num_ops;
    r.elapsed_seconds = dt.count();
    r.ns_per_op       = dt.count() * 1e9 / static_cast<double>(num_ops);
    r.ops_per_second  = static_cast<double>(num_ops) / dt.count();
    r.trades_emitted  = book.trades().size() - trades_before;
    return r;
}
