# orderbook-latency

A limit order book implemented two ways, benchmarked under identical load to
isolate the cost of the data-structure choice.

- **`MapOrderBook`** — `std::map` keyed by price. O(log n) per op; each price
  level is a separately-allocated tree node, so ops chase pointers across the
  heap.
- **`FlatOrderBook`** — prices are integer ticks indexing a contiguous vector.
  O(1) add/cancel, cache-friendly, best price tracked incrementally.

Both expose the same `add / cancel / best_bid / best_ask` interface. The
benchmark runs the same synthetic add+cancel workload against each and reports
per-op latency percentiles plus an ASCII distribution.

## Results

1M add + 1M cancel ops, 20k price levels (GCC 13, -O2, Linux):

| Op          | std::map | flat vector | speedup |
|-------------|----------|-------------|---------|
| add (mean)  | 482 ns   | 65 ns       | ~7.4x   |
| cancel+best | 549 ns   | 67 ns       | ~8.2x   |

The flat book also has a much tighter tail — the map pays allocator and
cache-miss cost that shows up beyond p99.

## Build & run
```sh
cmake -B build && cmake --build build && ./build/bench_orderbook
```
Single-threaded, so numbers are stable on any machine.
