// LobsterReplay.h
#ifndef LOBSTER_REPLAY_H
#define LOBSTER_REPLAY_H

#include "OrderBook.h"
#include <string>
#include <cstdint>
#include <vector>
#include <memory>
#include <functional>
#include <iosfwd>

struct ReplayStats {
    uint64_t submissions = 0;
    uint64_t partial_cancels = 0;
    uint64_t full_deletes = 0;
    uint64_t visible_execs = 0;
    uint64_t hidden_execs = 0;
    uint64_t halts = 0;
    uint64_t trades_emitted = 0;
    uint64_t rows_processed = 0;
    double elapsed_seconds = 0.0;
};

struct DatasetSpec {
    std::string label;
    std::string csv_path;
};

struct LabeledStats {
    std::string label;
    ReplayStats stats;
};

class LobsterReplay {
public:
    static ReplayStats run(const std::string& csv_path,
                           OrderBook& book,
                           uint64_t max_rows = 0,
                           bool verbose = false);

    static std::vector<LabeledStats> run_multi(
        const std::vector<DatasetSpec>& datasets,
        const std::function<std::unique_ptr<OrderBook>()>& book_factory,
        uint64_t max_rows = 0,
        bool verbose = false);

    static void print_comparison(const std::vector<LabeledStats>& results,
                                 std::ostream& os);
};

#endif // LOBSTER_REPLAY_H
