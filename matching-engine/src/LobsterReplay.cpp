// LobsterReplay.cpp
// -----------------------------------------------------------------------------

#include "LobsterReplay.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <cstring>

// Hand-rolled fast CSV field splitter. std::getline per field is slower than
// a single scan over the line, and this file has hundreds of thousands of rows.
static bool split_six(const std::string& line,
                      double& t, int& type, uint64_t& oid,
                      uint32_t& size, int64_t& price, int& direction) {
    const char* p = line.c_str();
    char* end = nullptr;

    t = std::strtod(p, &end);
    if (end == p || *end != ',') return false;
    p = end + 1;

    type = static_cast<int>(std::strtol(p, &end, 10));
    if (end == p || *end != ',') return false;
    p = end + 1;

    oid = std::strtoull(p, &end, 10);
    if (end == p || *end != ',') return false;
    p = end + 1;

    size = static_cast<uint32_t>(std::strtoul(p, &end, 10));
    if (end == p || *end != ',') return false;
    p = end + 1;

    price = std::strtoll(p, &end, 10);
    if (end == p || *end != ',') return false;
    p = end + 1;

    direction = static_cast<int>(std::strtol(p, &end, 10));
    if (end == p) return false;
    return true;
}

ReplayStats LobsterReplay::run(const std::string& csv_path,
                               OrderBook& book,
                               uint64_t max_rows,
                               bool verbose) {
    ReplayStats stats;
    std::ifstream in(csv_path);
    if (!in) {
        std::cerr << "ERROR: cannot open " << csv_path << "\n";
        return stats;
    }

    std::string line;
    line.reserve(128);
    auto t0 = std::chrono::high_resolution_clock::now();

    while (std::getline(in, line)) {
        if (line.empty()) continue;

        double   t;
        int      type;
        uint64_t oid;
        uint32_t size;
        int64_t  price;
        int      direction;

        if (!split_six(line, t, type, oid, size, price, direction)) {
            std::cerr << "WARN: malformed row skipped: " << line << "\n";
            continue;
        }

        Side side = (direction == 1) ? Side::BUY : Side::SELL;

        switch (type) {
            case 1:   // New limit order submission
                book.add_order(oid, side, price, size, t);
                ++stats.submissions;
                break;
            case 2:   // Partial cancellation (size = shares removed)
                book.reduce_order(oid, size);
                ++stats.partial_cancels;
                break;
            case 3:   // Full deletion
                book.cancel_order(oid);
                ++stats.full_deletes;
                break;
            case 4:   // Execution of a visible limit order
                book.execute_order(oid, size, t);
                ++stats.visible_execs;
                break;
            case 5:   // Execution of a hidden order (not in our book by def'n)
                ++stats.hidden_execs;
                break;
            case 7:   // Trading halt / quoting / resume — no book mutation
                ++stats.halts;
                break;
            default:
                // Unknown type; ignore
                break;
        }

        ++stats.rows_processed;
        if (max_rows && stats.rows_processed >= max_rows) break;

        if (verbose && stats.rows_processed % 100000 == 0) {
            std::cout << "  ... " << stats.rows_processed << " rows replayed\n";
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> dt = t1 - t0;
    stats.elapsed_seconds = dt.count();
    stats.trades_emitted  = book.trades().size();
    return stats;
}
