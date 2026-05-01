
// Implementation of the matching engine described in OrderBook.h.


#include "OrderBook.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

OrderBook::OrderBook() {
   
    order_index_.reserve(1 << 16);
    trades_.reserve(1 << 14);
}

OrderBook::~OrderBook() {
 
    for (auto& kv : order_index_) delete kv.second;
    order_index_.clear();
}


template <typename BookSide>
void OrderBook::match_against(Order& incoming, BookSide& opposite) {
    while (!opposite.empty() && incoming.quantity > 0) {
        auto best_it = opposite.begin();   // "best" is always at begin()
        int64_t best_price = best_it->first;

       
        bool crosses = (incoming.side == Side::BUY)
                         ? (incoming.price >= best_price)
                         : (incoming.price <= best_price);
        if (!crosses) break;

        PriceLevel& level = best_it->second;

        
        while (!level.empty() && incoming.quantity > 0) {
            Order* resting  = level.head();
            uint32_t fill   = std::min(incoming.quantity, resting->quantity);

         
            trades_.push_back({
                resting->order_id,
                incoming.order_id,
                resting->price,
                fill,
                incoming.timestamp
            });

            incoming.quantity  -= fill;
            resting->quantity  -= fill;
            level.total_volume -= fill;

            if (resting->quantity == 0) {

                uint64_t rid = resting->order_id;
                level.remove_head();   
              
                order_index_.erase(rid);
                delete resting;
            }
        }

        
        if (level.empty()) {
            opposite.erase(best_it);
        }
    }
}


void OrderBook::add_order(uint64_t order_id, Side side, int64_t price,
                          uint32_t quantity, double timestamp) {
    if (quantity == 0) return;

    Order working(order_id, side, price, quantity, timestamp);

    if (side == Side::BUY) match_against(working, asks_);
    else                   match_against(working, bids_);

    if (working.quantity == 0) return;  // Fully filled by matching.

 
    Order* resting = new Order(order_id, side, price, working.quantity, timestamp);

    if (side == Side::BUY) {
        auto it = bids_.find(price);
        if (it == bids_.end())
            it = bids_.emplace(price, PriceLevel(price)).first;
        it->second.append_tail(resting);
    } else {
        auto it = asks_.find(price);
        if (it == asks_.end())
            it = asks_.emplace(price, PriceLevel(price)).first;
        it->second.append_tail(resting);
    }

    order_index_[order_id] = resting;
}


void OrderBook::cancel_order(uint64_t order_id) {
    auto it = order_index_.find(order_id);
    if (it == order_index_.end()) return;    // Unknown id — ignore.

    Order* o = it->second;

    if (o->side == Side::BUY) {
        auto lv = bids_.find(o->price);
        if (lv != bids_.end()) {
            lv->second.remove(o);
            if (lv->second.empty()) bids_.erase(lv);
        }
    } else {
        auto lv = asks_.find(o->price);
        if (lv != asks_.end()) {
            lv->second.remove(o);
            if (lv->second.empty()) asks_.erase(lv);
        }
    }

    order_index_.erase(it);
    delete o;
}


void OrderBook::reduce_order(uint64_t order_id, uint32_t cancelled_qty) {
    auto it = order_index_.find(order_id);
    if (it == order_index_.end()) return;

    Order* o = it->second;
    if (cancelled_qty >= o->quantity) {
        
        cancel_order(order_id);
        return;
    }
    o->quantity -= cancelled_qty;

    
    if (o->side == Side::BUY) {
        auto lv = bids_.find(o->price);
        if (lv != bids_.end()) lv->second.total_volume -= cancelled_qty;
    } else {
        auto lv = asks_.find(o->price);
        if (lv != asks_.end()) lv->second.total_volume -= cancelled_qty;
    }
}


void OrderBook::execute_order(uint64_t order_id, uint32_t exec_qty, double ts) {
    auto it = order_index_.find(order_id);
    if (it == order_index_.end()) return;

    Order* o = it->second;
    uint32_t fill = std::min(exec_qty, o->quantity);

    trades_.push_back({o->order_id, 0 /*unknown aggressor*/, o->price, fill, ts});

    if (fill >= o->quantity) {
        cancel_order(order_id);   
    } else {
        o->quantity -= fill;
        if (o->side == Side::BUY) {
            auto lv = bids_.find(o->price);
            if (lv != bids_.end()) lv->second.total_volume -= fill;
        } else {
            auto lv = asks_.find(o->price);
            if (lv != asks_.end()) lv->second.total_volume -= fill;
        }
    }
}


bool OrderBook::has_order(uint64_t order_id) const {
    return order_index_.find(order_id) != order_index_.end();
}

int64_t OrderBook::best_bid() const {
    return bids_.empty() ? 0 : bids_.begin()->first;
}

int64_t OrderBook::best_ask() const {
    return asks_.empty() ? 0 : asks_.begin()->first;
}

void OrderBook::print_top_of_book(std::ostream& os) const {
    os << "BBO  bid=";
    if (bids_.empty()) os << "---";
    else os << std::fixed << std::setprecision(4)
            << (bids_.begin()->first / 10000.0)
            << " x " << bids_.begin()->second.total_volume;
    os << "   ask=";
    if (asks_.empty()) os << "---";
    else os << std::fixed << std::setprecision(4)
            << (asks_.begin()->first / 10000.0)
            << " x " << asks_.begin()->second.total_volume;
    os << "\n";
}

void OrderBook::print_depth(std::ostream& os, size_t levels) const {
    os << "---- Order Book Depth (top " << levels << ") ----\n";
    os << std::left << std::setw(20) << "BIDS (price x qty)"
       << "     " << "ASKS (price x qty)" << "\n";

    auto bit = bids_.begin();
    auto ait = asks_.begin();
    for (size_t i = 0; i < levels; ++i) {
        std::string bidstr = "---";
        std::string askstr = "---";
        if (bit != bids_.end()) {
            bidstr = std::to_string(bit->first / 10000.0).substr(0, 8) +
                     " x " + std::to_string(bit->second.total_volume);
            ++bit;
        }
        if (ait != asks_.end()) {
            askstr = std::to_string(ait->first / 10000.0).substr(0, 8) +
                     " x " + std::to_string(ait->second.total_volume);
            ++ait;
        }
        os << std::left << std::setw(20) << bidstr << "     " << askstr << "\n";
    }
}
