// LobsterReplay.h
// -----------------------------------------------------------------------------
// Reads a LOBSTER "message" CSV file and replays it through an OrderBook.
//
// LOBSTER message file columns:
//   1. Time        (seconds after midnight, fractional)
//   2. Type        1=submission, 2=partial cancel, 3=deletion,
//                  4=visible execution, 5=hidden execution, 7=halt
//   3. Order ID
//   4. Size        (shares)
//   5. Price       (dollars * 10000)
//   6. Direction   (1=buy, -1=sell)
// -----------------------------------------------------------------------------

#ifndef LOBSTER_REPLAY_H
#define LOBSTER_REPLAY_H

#include "OrderBook.h"
#include <string>
#include <cstdint>

struct ReplayStats {
    uint64_t submissions      = 0;
    uint64_t partial_cancels  = 0;
    uint64_t full_deletes     = 0;
    uint64_t visible_execs    = 0;
    uint64_t hidden_execs     = 0;
    uint64_t halts            = 0;
    uint64_t trades_emitted   = 0;
    uint64_t rows_processed   = 0;
    double   elapsed_seconds  = 0.0;
};

class LobsterReplay {
public:
    // Replay every row in `csv_path` through `book`. If `max_rows` > 0, stop
    // after that many rows (useful for quick correctness checks).
    static ReplayStats run(const std::string& csv_path,
                           OrderBook& book,
                           uint64_t max_rows = 0,
                           bool verbose = false);
};

#endif // LOBSTER_REPLAY_H
