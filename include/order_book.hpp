#pragma once
#include <cstdint>
#include <map>
#include <vector>

enum class Side { Bid, Ask };

// std::map keyed by price: O(log n), node-per-level, pointer chasing
struct MapOrderBook {
    struct Order { std::int64_t price; Side side; std::uint32_t qty; };
    std::map<std::int64_t, std::uint64_t, std::greater<>> bid_qty;
    std::map<std::int64_t, std::uint64_t> ask_qty;
    std::vector<Order> orders;

    explicit MapOrderBook(std::size_t max_orders) { orders.resize(max_orders); }

    void add(std::uint32_t id, Side side, std::int64_t price, std::uint32_t qty) {
        orders[id] = {price, side, qty};
        if (side == Side::Bid) bid_qty[price] += qty;
        else                   ask_qty[price] += qty;
    }
    void cancel(std::uint32_t id) {
        Order& o = orders[id];
        if (o.qty == 0) return;
        if (o.side == Side::Bid) {
            auto it = bid_qty.find(o.price);
            if (it != bid_qty.end() && (it->second -= o.qty) == 0) bid_qty.erase(it);
        } else {
            auto it = ask_qty.find(o.price);
            if (it != ask_qty.end() && (it->second -= o.qty) == 0) ask_qty.erase(it);
        }
        o.qty = 0;
    }
    std::int64_t best_bid() const { return bid_qty.empty() ? -1 : bid_qty.begin()->first; }
    std::int64_t best_ask() const { return ask_qty.empty() ? -1 : ask_qty.begin()->first; }
};

// price = tick index into a flat vector: O(1), contiguous, cache-friendly
struct FlatOrderBook {
    struct Order { std::int64_t price; Side side; std::uint32_t qty; };
    std::vector<std::uint64_t> bid_qty;
    std::vector<std::uint64_t> ask_qty;
    std::vector<Order> orders;
    std::int64_t best_bid_ = -1;
    std::int64_t best_ask_ = -1;
    std::int64_t max_tick_;

    FlatOrderBook(std::size_t max_orders, std::int64_t max_tick)
        : bid_qty(max_tick + 1, 0), ask_qty(max_tick + 1, 0),
          orders(max_orders), max_tick_(max_tick) {}

    void add(std::uint32_t id, Side side, std::int64_t price, std::uint32_t qty) {
        orders[id] = {price, side, qty};
        if (side == Side::Bid) {
            bid_qty[price] += qty;
            if (price > best_bid_) best_bid_ = price;
        } else {
            ask_qty[price] += qty;
            if (best_ask_ < 0 || price < best_ask_) best_ask_ = price;
        }
    }
    void cancel(std::uint32_t id) {
        Order& o = orders[id];
        if (o.qty == 0) return;
        if (o.side == Side::Bid) {
            bid_qty[o.price] -= o.qty;
            if (o.price == best_bid_) while (best_bid_ >= 0 && bid_qty[best_bid_] == 0) --best_bid_;
        } else {
            ask_qty[o.price] -= o.qty;
            if (o.price == best_ask_) {
                while (best_ask_ <= max_tick_ && ask_qty[best_ask_] == 0) ++best_ask_;
                if (best_ask_ > max_tick_) best_ask_ = -1;
            }
        }
        o.qty = 0;
    }
    std::int64_t best_bid() const { return best_bid_; }
    std::int64_t best_ask() const { return best_ask_; }
};
