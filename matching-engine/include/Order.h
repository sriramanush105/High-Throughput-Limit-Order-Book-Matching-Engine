
// Represents a single limit order resting in the book.
// Prices are stored as int64_t in "LOBSTER integer format" (dollar price * 10000)


#ifndef ORDER_H
#define ORDER_H

#include <cstdint>

enum class Side : int8_t {
    BUY  = 1,
    SELL = -1
};

struct Order {
    uint64_t order_id;    
    Side     side;         
    int64_t  price;        
    uint32_t quantity;     
    double   timestamp;   

    //  doubly-linked-list pointers so each order knows its neighbours at its price level.
    Order* prev = nullptr;
    Order* next = nullptr;

    Order() = default;
    Order(uint64_t id, Side s, int64_t p, uint32_t q, double t)
        : order_id(id), side(s), price(p), quantity(q), timestamp(t) {}
};

#endif // ORDER_H
