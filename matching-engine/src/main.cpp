// main.cpp
// -----------------------------------------------------------------------------
// Entry point. Supports two modes:
//   ./matching_engine replay  <csv>  [max_rows]
//   ./matching_engine bench   [num_ops]
// With no arguments, prints usage and runs a tiny built-in demo.
// -----------------------------------------------------------------------------

#include "OrderBook.h"
#include "LobsterReplay.h"
#include "Benchmark.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>

static void print_usage(const char* prog) {
    std::cout <<
    "\nHigh-Throughput Limit Order Book Matching Engine\n"
    "-------------------------------------------------\n"
    "Usage:\n"
    "  " << prog << " replay <csv_path> [max_rows]\n"
    "      Replay a LOBSTER message file through the engine.\n\n"
    "  " << prog << " bench [num_ops]\n"
    "      Run a synthetic benchmark (default 1,000,000 operations).\n\n"
    "  " << prog << " demo\n"
    "      Run a small hand-crafted demo showing matching behaviour.\n\n";
}

static void run_demo() {
    std::cout << "\n===== Built-in Demo =====\n";
    OrderBook book;

    // Build a simple book
    book.add_order(1, Side::BUY,  310000, 100, 34200.0);   // bid 31.00 x 100
    book.add_order(2, Side::BUY,  309900, 200, 34200.1);   // bid 30.99 x 200
    book.add_order(3, Side::SELL, 310200, 150, 34200.2);   // ask 31.02 x 150
    book.add_order(4, Side::SELL, 310300,  50, 34200.3);   // ask 31.03 x 50

    std::cout << "\nInitial book:\n";
    book.print_depth(std::cout, 3);
    book.print_top_of_book(std::cout);

    // Aggressive buy that crosses the ask -> should trade 150 @ 31.02
    std::cout << "\n>> Incoming aggressive BUY 250 @ 31.02\n";
    book.add_order(5, Side::BUY, 310200, 250, 34200.4);

    std::cout << "\nAfter match:\n";
    book.print_depth(std::cout, 3);
    book.print_top_of_book(std::cout);

    std::cout << "\nTrade log (" << book.trades().size() << " trades):\n";
    for (const auto& tr : book.trades()) {
        std::cout << "  taker=" << tr.incoming_id
                  << " maker=" << tr.resting_id
                  << "  " << tr.quantity << " @ "
                  << std::fixed << std::setprecision(4)
                  << (tr.price / 10000.0) << "\n";
    }

    // Cancel order 2
    std::cout << "\n>> Cancel order 2\n";
    book.cancel_order(2);
    book.print_depth(std::cout, 3);
    book.print_top_of_book(std::cout);
}

static int run_replay(const std::string& csv, uint64_t max_rows) {
    std::cout << "\n===== LOBSTER Replay =====\n";
    std::cout << "Input file : " << csv << "\n";
    if (max_rows) std::cout << "Max rows   : " << max_rows << "\n";

    OrderBook book;
    ReplayStats s = LobsterReplay::run(csv, book, max_rows, true);

    std::cout << "\n---- Replay Summary ----\n";
    std::cout << "Rows processed    : " << s.rows_processed << "\n";
    std::cout << "  submissions     : " << s.submissions << "\n";
    std::cout << "  partial cancels : " << s.partial_cancels << "\n";
    std::cout << "  full deletes    : " << s.full_deletes << "\n";
    std::cout << "  visible execs   : " << s.visible_execs << "\n";
    std::cout << "  hidden execs    : " << s.hidden_execs << "\n";
    std::cout << "  halts           : " << s.halts << "\n";
    std::cout << "Trades emitted    : " << s.trades_emitted << "\n";
    std::cout << "Elapsed           : " << std::fixed << std::setprecision(3)
              << s.elapsed_seconds << " s\n";
    if (s.elapsed_seconds > 0) {
        std::cout << "Throughput        : "
                  << std::fixed << std::setprecision(0)
                  << (s.rows_processed / s.elapsed_seconds)
                  << " msgs/sec\n";
        std::cout << "Latency           : "
                  << std::fixed << std::setprecision(1)
                  << (s.elapsed_seconds * 1e9 / s.rows_processed)
                  << " ns/msg\n";
    }

    std::cout << "\nFinal book state:\n";
    std::cout << "  bid levels      : " << book.bid_levels() << "\n";
    std::cout << "  ask levels      : " << book.ask_levels() << "\n";
    std::cout << "  live orders     : " << book.live_orders() << "\n";
    book.print_top_of_book(std::cout);
    book.print_depth(std::cout, 5);
    return 0;
}

static int run_bench(uint64_t num_ops) {
    std::cout << "\n===== Synthetic Benchmark =====\n";
    std::cout << "Operations : " << num_ops << "\n";

    OrderBook book;
    BenchmarkResult r = Benchmark::run(book, num_ops);

    std::cout << "\n---- Benchmark Result ----\n";
    std::cout << "Elapsed        : " << std::fixed << std::setprecision(3)
              << r.elapsed_seconds << " s\n";
    std::cout << "Throughput     : " << std::fixed << std::setprecision(0)
              << r.ops_per_second << " ops/sec\n";
    std::cout << "Avg latency    : " << std::fixed << std::setprecision(1)
              << r.ns_per_op << " ns/op\n";
    std::cout << "Trades emitted : " << r.trades_emitted << "\n";
    std::cout << "Live orders    : " << book.live_orders() << "\n";
    book.print_top_of_book(std::cout);
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        run_demo();
        return 0;
    }

    std::string cmd = argv[1];
    if (cmd == "replay") {
        if (argc < 3) { print_usage(argv[0]); return 1; }
        uint64_t max_rows = (argc >= 4) ? std::strtoull(argv[3], nullptr, 10) : 0;
        return run_replay(argv[2], max_rows);
    }
    if (cmd == "bench") {
        uint64_t n = (argc >= 3) ? std::strtoull(argv[2], nullptr, 10) : 1'000'000ULL;
        return run_bench(n);
    }
    if (cmd == "demo") {
        run_demo();
        return 0;
    }
    print_usage(argv[0]);
    return 1;
}
