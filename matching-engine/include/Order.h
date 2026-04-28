// Order.h
// -----------------------------------------------------------------------------
// Represents a single limit order resting in the book.
// Prices are stored as int64_t in "LOBSTER integer format" (dollar price * 10000)
// to completely avoid floating-point comparison issues in price-level bucketing.
// -----------------------------------------------------------------------------

#ifndef ORDER_H
#define ORDER_H

#include <cstdint>

enum class Side : int8_t {
    BUY  = 1,
    SELL = -1
};

struct Order {
    uint64_t order_id;     // Unique ID assigned by the exchange
    Side     side;         // BUY or SELL
    int64_t  price;        // Integer price (dollars * 10000)
    uint32_t quantity;     // Remaining shares
    double   timestamp;    // Seconds after midnight, with fractional precision

    // Intrusive doubly-linked-list pointers so each order knows its neighbours at its price level. This gives O(1) cancellation by order_id.
    Order* prev = nullptr;
    Order* next = nullptr;

    Order() = default;
    Order(uint64_t id, Side s, int64_t p, uint32_t q, double t)
        : order_id(id), side(s), price(p), quantity(q), timestamp(t) {}
};

#endif // ORDER_H
