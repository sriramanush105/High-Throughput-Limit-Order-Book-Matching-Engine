// Maintains two sorted maps of price levels
// bids are sorted high→low, asks sorted low→high 


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


struct Trade {
    uint64_t resting_id;     
    uint64_t incoming_id;    
    int64_t  price;          
    uint32_t quantity;       
    double   timestamp;     
};

class OrderBook {
public:
    OrderBook();
    ~OrderBook();

  
    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;

   
    void add_order(uint64_t order_id, Side side, int64_t price,
                   uint32_t quantity, double timestamp);
    void cancel_order(uint64_t order_id);
    void reduce_order(uint64_t order_id, uint32_t cancelled_qty);  
    void execute_order(uint64_t order_id, uint32_t exec_qty,
                       double timestamp);                         

    
    bool    has_order(uint64_t order_id) const;
    int64_t best_bid() const;   // returns 0 if empty
    int64_t best_ask() const;   // returns 0 if empty
    size_t  bid_levels() const { return bids_.size(); }
    size_t  ask_levels() const { return asks_.size(); }
    size_t  live_orders() const { return order_index_.size(); }

    
    const std::vector<Trade>& trades() const { return trades_; }
    void clear_trades() { trades_.clear(); }

    void print_top_of_book(std::ostream& os) const;
    
    void print_depth(std::ostream& os, size_t levels = 5) const;

private:
    
    std::map<int64_t, PriceLevel, std::greater<int64_t>> bids_;
    std::map<int64_t, PriceLevel, std::less<int64_t>>    asks_;

    
    std::unordered_map<uint64_t, Order*> order_index_;

    
    std::vector<Trade> trades_;

   
    template <typename BookSide>
    void match_against(Order& incoming, BookSide& opposite);
};

#endif // ORDER_BOOK_H
