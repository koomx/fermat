#ifndef COUNTERS__EVENT_COUNTER_H
#define COUNTERS__EVENT_COUNTER_H

#include <chrono>
#include <vector>

namespace counters {

struct event_count {
    std::chrono::duration<double> elapsed{};
    std::vector<double> event_counts{0, 0, 0, 0, 0};

    enum event_counter_types {
        CPU_CYCLES,
        INSTRUCTIONS,
        BRANCH,
        BRANCH_MISSES,
        CACHE_MISSES
    };

    double elapsed_ns() const {
        return std::chrono::duration<double, std::nano>(elapsed).count();
    }
    double cycles() const { return event_counts[CPU_CYCLES]; }
    double instructions() const { return event_counts[INSTRUCTIONS]; }
    double branch_misses() const { return event_counts[BRANCH_MISSES]; }
    double branches() const { return event_counts[BRANCH]; }
    double cache_misses() const { return event_counts[CACHE_MISSES]; }
};

struct event_aggregate {
    bool has_events = false;
    int iterations = 0;
    int inner_count = 1;
    event_count total{};
    event_count best{};
    event_count worst{};

    void operator<<(const event_count& other) {
        if (iterations == 0 || other.elapsed < best.elapsed) {
            best = other;
        }
        if (iterations == 0 || other.elapsed > worst.elapsed) {
            worst = other;
        }
        iterations++;
        total.elapsed += other.elapsed;
        for (size_t i = 0; i < total.event_counts.size(); i++) {
            total.event_counts[i] += other.event_counts[i];
        }
    }

    double fastest_elapsed_ns() const { return best.elapsed_ns() / inner_count; }
    double fastest_cycles() const { return best.cycles() / inner_count; }
    double fastest_instructions() const {
        return best.instructions() / inner_count;
    }
    double fastest_branch_misses() const {
        return best.branch_misses() / inner_count;
    }
    double fastest_cache_misses() const {
        return best.cache_misses() / inner_count;
    }
};

struct event_collector {
    event_count count{};
    std::chrono::steady_clock::time_point start_clock{};

    bool has_events() { return false; }
    void start() { start_clock = std::chrono::steady_clock::now(); }
    event_count& end() {
        count.elapsed = std::chrono::steady_clock::now() - start_clock;
        return count;
    }
};

inline bool has_performance_counters() { return false; }

}  // namespace counters

#endif
