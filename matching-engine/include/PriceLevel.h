
// A single price level in the order book. Holds a FIFO queue of Orders at
// that exact price, implemented as a doubly-linked list so that:


#ifndef PRICE_LEVEL_H
#define PRICE_LEVEL_H

#include "Order.h"
#include <cstdint>

class PriceLevel {
public:
    int64_t  price        = 0;   
    uint32_t total_volume = 0;   
    uint32_t order_count  = 0;   

    PriceLevel() = default;
    explicit PriceLevel(int64_t p) : price(p) {}

    bool   empty() const { return head_ == nullptr; }
    Order* head()  const { return head_; }
    Order* tail()  const { return tail_; }

    
    void append_tail(Order* o) {
        o->prev = tail_;
        o->next = nullptr;
        if (tail_) tail_->next = o;
        else       head_ = o;     
        tail_ = o;
        total_volume += o->quantity;
        ++order_count;
    }

    
    void remove_head() {
        if (!head_) return;
        Order* h = head_;
        head_ = h->next;
        if (head_) head_->prev = nullptr;
        else       tail_ = nullptr;
        total_volume -= h->quantity;
        --order_count;
    }

 
    void remove(Order* o) {
        if (o->prev) o->prev->next = o->next;
        else         head_         = o->next;
        if (o->next) o->next->prev = o->prev;
        else         tail_         = o->prev;
        total_volume -= o->quantity;
        --order_count;
    }

private:
    Order* head_ = nullptr;
    Order* tail_ = nullptr;
};

#endif 
