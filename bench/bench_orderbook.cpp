#include "../include/order_book.hpp"
#include "../include/latency_histogram.hpp"
#include "../include/timing.hpp"
#include <cstdio>
#include <random>
#include <string>
#include <vector>

static constexpr std::size_t kOrders = 1'000'000;
static constexpr std::int64_t kMaxTick = 20'000;
static constexpr std::int64_t kSpread  = 100;

template <typename Book>
static void run(Book& book, const char* label,
                const std::vector<std::int64_t>& prices,
                const std::vector<Side>& sides) {
    LatencyHistogram add_h(kOrders), cancel_h(kOrders);
    for (std::uint32_t id = 0; id < kOrders; ++id) {
        std::uint64_t t0 = now_ns();
        book.add(id, sides[id], prices[id], 10);
        add_h.record(now_ns() - t0);
    }
    volatile std::int64_t sink = 0;
    for (std::uint32_t id = 0; id < kOrders; ++id) {
        std::uint64_t t0 = now_ns();
        book.cancel(id);
        sink += book.best_bid() + book.best_ask();
        cancel_h.record(now_ns() - t0);
    }
    add_h.report((std::string(label) + " : add").c_str());
    cancel_h.report((std::string(label) + " : cancel+best").c_str());
}

int main() {
    std::mt19937 rng(42);
    std::uniform_int_distribution<std::int64_t> mid(kSpread, kMaxTick - kSpread);
    std::uniform_int_distribution<int> off(0, kSpread);
    std::vector<std::int64_t> prices(kOrders);
    std::vector<Side> sides(kOrders);
    for (std::size_t i = 0; i < kOrders; ++i) {
        std::int64_t m = mid(rng);
        if (i & 1) { sides[i] = Side::Bid; prices[i] = m - off(rng); }
        else       { sides[i] = Side::Ask; prices[i] = m + off(rng); }
    }
    std::printf("Order book: %zu add + %zu cancel ops, %lld price levels\n",
                kOrders, kOrders, (long long)kMaxTick);
    { MapOrderBook  b(kOrders);           run(b, "std::map book",  prices, sides); }
    { FlatOrderBook b(kOrders, kMaxTick); run(b, "flat vector book", prices, sides); }
    return 0;
}
