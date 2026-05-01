// LobsterReplay.cpp
#include "LobsterReplay.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>

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

        double t;
        int type;
        uint64_t oid;
        uint32_t size;
        int64_t price;
        int direction;

        if (!split_six(line, t, type, oid, size, price, direction)) {
            std::cerr << "WARN: malformed row skipped: " << line << "\n";
            continue;
        }

        Side side = (direction == 1) ? Side::BUY : Side::SELL;

        switch (type) {
            case 1:
                book.add_order(oid, side, price, size, t);
                ++stats.submissions;
                break;
            case 2:
                book.reduce_order(oid, size);
                ++stats.partial_cancels;
                break;
            case 3:
                book.cancel_order(oid);
                ++stats.full_deletes;
                break;
            case 4:
                book.execute_order(oid, size, t);
                ++stats.visible_execs;
                break;
            case 5:
                ++stats.hidden_execs;
                break;
            case 7:
                ++stats.halts;
                break;
            default:
                break;
        }

        ++stats.rows_processed;
        if (max_rows && stats.rows_processed >= max_rows) break;

        if (verbose && stats.rows_processed % 100000 == 0) {
            std::cout << " ... " << stats.rows_processed << " rows replayed\n";
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> dt = t1 - t0;
    stats.elapsed_seconds = dt.count();
    stats.trades_emitted = book.trades().size();

    return stats;
}

std::vector<LabeledStats>
LobsterReplay::run_multi(const std::vector<DatasetSpec>& datasets,
                         const std::function<std::unique_ptr<OrderBook>()>& book_factory,
                         uint64_t max_rows,
                         bool verbose) {
    std::vector<LabeledStats> results;
    results.reserve(datasets.size());

    for (const auto& ds : datasets) {
        if (verbose) {
            std::cout << "=== Replaying " << ds.label
                      << " (" << ds.csv_path << ") ===\n";
        }

        auto book = book_factory();
        if (!book) {
            std::cerr << "ERROR: book_factory returned null for " << ds.label << "\n";
            continue;
        }

        ReplayStats s = run(ds.csv_path, *book, max_rows, verbose);
        results.push_back({ds.label, s});

        if (verbose) {
            std::cout << " done: " << s.rows_processed << " rows in "
                      << std::fixed << std::setprecision(3)
                      << s.elapsed_seconds << " s\n\n";
        }
    }

    return results;
}

void LobsterReplay::print_comparison(const std::vector<LabeledStats>& results,
                                     std::ostream& os) {
    if (results.empty()) {
        os << "(no datasets replayed)\n";
        return;
    }

    auto hdr = [&](const char* name) {
        os << std::left << std::setw(20) << name;
        for (const auto& r : results) 
            os << std::right << std::setw(16) << r.label;
        os << "\n";
    };

    auto row_u = [&](const char* name, auto getter) {
        os << std::left << std::setw(20) << name;
        for (const auto& r : results) 
            os << std::right << std::setw(16) << getter(r.stats);
        os << "\n";
    };

    auto row_d = [&](const char* name, auto getter) {
        os << std::left << std::setw(20) << name;
        for (const auto& r : results)
            os << std::right << std::setw(16) << std::fixed << std::setprecision(3) 
               << getter(r.stats);
        os << "\n";
    };

    os << "\n--- Replay comparison ---\n";
    hdr("metric");
    os << std::string(20 + 16 * results.size(), '-') << "\n";

    row_u("rows_processed",    [](const ReplayStats& s){ return s.rows_processed; });
    row_u("submissions",       [](const ReplayStats& s){ return s.submissions; });
    row_u("partial_cancels",   [](const ReplayStats& s){ return s.partial_cancels; });
    row_u("full_deletes",      [](const ReplayStats& s){ return s.full_deletes; });
    row_u("visible_execs",     [](const ReplayStats& s){ return s.visible_execs; });
    row_u("hidden_execs",      [](const ReplayStats& s){ return s.hidden_execs; });
    row_u("halts",             [](const ReplayStats& s){ return s.halts; });
    row_u("trades_emitted",    [](const ReplayStats& s){ return s.trades_emitted; });
    row_d("elapsed_seconds",   [](const ReplayStats& s){ return s.elapsed_seconds; });
    row_d("rows_per_second",   [](const ReplayStats& s){
        return s.elapsed_seconds > 0.0 
             ? static_cast<double>(s.rows_processed) / s.elapsed_seconds 
             : 0.0;
    });

    os << "\n";
}
