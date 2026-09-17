#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

class LatencyHistogram {
public:
    explicit LatencyHistogram(std::size_t expected = 1'000'000) {
        samples_.reserve(expected);
    }

    void record(std::uint64_t ns) { samples_.push_back(ns); }
    std::size_t count() const { return samples_.size(); }

    std::uint64_t pct(double p) {
        if (samples_.empty()) return 0;
        ensure_sorted();
        std::size_t idx = static_cast<std::size_t>((p / 100.0) * (samples_.size() - 1));
        return samples_[idx];
    }
    std::uint64_t min() { ensure_sorted(); return samples_.front(); }
    std::uint64_t max() { ensure_sorted(); return samples_.back(); }

    double mean() const {
        if (samples_.empty()) return 0.0;
        long double sum = 0;
        for (auto s : samples_) sum += s;
        return static_cast<double>(sum / samples_.size());
    }

    void report(const char* label) {
        if (samples_.empty()) { std::printf("%s: no samples\n", label); return; }
        ensure_sorted();
        std::printf("\n== %s (n=%zu) ==\n", label, samples_.size());
        std::printf("  mean   %8.1f ns\n", mean());
        std::printf("  p50    %8llu ns\n", (unsigned long long)pct(50));
        std::printf("  p90    %8llu ns\n", (unsigned long long)pct(90));
        std::printf("  p99    %8llu ns\n", (unsigned long long)pct(99));
        std::printf("  p99.9  %8llu ns\n", (unsigned long long)pct(99.9));
        std::printf("  max    %8llu ns\n", (unsigned long long)max());
        print_ascii();
    }

private:
    void ensure_sorted() {
        if (!sorted_) { std::sort(samples_.begin(), samples_.end()); sorted_ = true; }
    }

    void print_ascii() {
        std::uint64_t lo = pct(1), hi = pct(99);
        if (hi <= lo) hi = lo + 1;
        const int kBuckets = 20;
        std::vector<std::size_t> buckets(kBuckets, 0);
        double width = static_cast<double>(hi - lo) / kBuckets;
        for (auto s : samples_) {
            if (s < lo || s > hi) continue;
            int b = static_cast<int>((s - lo) / width);
            if (b >= kBuckets) b = kBuckets - 1;
            buckets[b]++;
        }
        std::size_t peak = *std::max_element(buckets.begin(), buckets.end());
        if (peak == 0) return;
        std::printf("  distribution (p1..p99, ns):\n");
        for (int i = 0; i < kBuckets; ++i) {
            std::uint64_t edge = lo + static_cast<std::uint64_t>(i * width);
            int bars = static_cast<int>(50.0 * buckets[i] / peak);
            std::printf("  %8llu | %s\n", (unsigned long long)edge, std::string(bars, '#').c_str());
        }
    }

    std::vector<std::uint64_t> samples_;
    bool sorted_ = false;
};
