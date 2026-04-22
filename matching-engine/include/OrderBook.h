// OrderBook.h
// -----------------------------------------------------------------------------
// The matching engine itself. Maintains two sorted maps of price levels
// (bids sorted high→low, asks sorted low→high) plus a hash map from order_id
// to Order* for O(1) cancellation and modification.
//
// Matching rules implemented (classic Price-Time priority):
//   1. Price priority : higher bids and lower asks match first
//   2. Time  priority : at the same price, earlier orders fill first
// -----------------------------------------------------------------------------

#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include "Order.h"
#include "PriceLevel.h"
#include <map>
#include <unordered_map>
#include <functional>
#include <cstdint>
#include <vector>
#include <iosfwd>

// A single executed trade — emitted whenever two orders cross.
struct Trade {
    uint64_t resting_id;     // Order resting in the book (maker)
    uint64_t incoming_id;    // Incoming aggressive order (taker)
    int64_t  price;          // Execution price (resting order's price wins)
    uint32_t quantity;       // Shares traded
    double   timestamp;      // Timestamp of the incoming order
};

class OrderBook {
public:
    OrderBook();
    ~OrderBook();

    // Disable copy; the book owns heap-allocated Orders.
    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;

    // ---- Core API (maps 1:1 to the pseudo-code) ----
    void add_order(uint64_t order_id, Side side, int64_t price,
                   uint32_t quantity, double timestamp);
    void cancel_order(uint64_t order_id);
    void reduce_order(uint64_t order_id, uint32_t cancelled_qty);  // LOBSTER type 2
    void execute_order(uint64_t order_id, uint32_t exec_qty,
                       double timestamp);                          // LOBSTER type 4

    // ---- Inspection ----
    bool    has_order(uint64_t order_id) const;
    int64_t best_bid() const;   // returns 0 if empty
    int64_t best_ask() const;   // returns 0 if empty
    size_t  bid_levels() const { return bids_.size(); }
    size_t  ask_levels() const { return asks_.size(); }
    size_t  live_orders() const { return order_index_.size(); }

    // ---- Trade log ----
    const std::vector<Trade>& trades() const { return trades_; }
    void clear_trades() { trades_.clear(); }

    // Pretty-print top-of-book to a stream (used by the BBO broadcaster).
    void print_top_of_book(std::ostream& os) const;
    // Pretty-print top N levels of both sides (for debugging / snapshots).
    void print_depth(std::ostream& os, size_t levels = 5) const;

private:
    // Bids sorted highest-first; asks sorted lowest-first.
    std::map<int64_t, PriceLevel, std::greater<int64_t>> bids_;
    std::map<int64_t, PriceLevel, std::less<int64_t>>    asks_;

    // order_id → pointer to the Order living inside some PriceLevel's list.
    std::unordered_map<uint64_t, Order*> order_index_;

    // Trade log produced by this session. Kept in memory; flushed on demand.
    std::vector<Trade> trades_;

    // Helpers
    template <typename BookSide>
    void match_against(Order& incoming, BookSide& opposite);
};

#endif // ORDER_BOOK_H
