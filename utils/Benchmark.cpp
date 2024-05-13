// Copyright (c) Qinetik 2024.

#include "Benchmark.h"
#include <chrono>
#include <functional>

std::string BenchmarkResults::representation() {
    std::string rep;
    rep.append("[Nano:");
    rep.append(std::to_string(nanos));
    rep.append("]Micro:");
    rep.append(std::to_string(micros));
    rep.append("][Milli:");
    rep.append(std::to_string(millis));
    rep.append("][Sec:");
    rep.append(std::to_string(seconds));
    rep.append("]");
    return rep;
}

BenchmarkResults benchmark(const std::function<void()>& fn) {
    auto start = std::chrono::steady_clock::now();

    // Actual fn
    fn();

    // Save end time
    auto end = std::chrono::steady_clock::now();

    // Calculating duration in different units
    return BenchmarkResults{
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count(),
            std::chrono::duration_cast<std::chrono::microseconds>(end - start).count(),
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(),
            std::chrono::duration_cast<std::chrono::seconds>(end - start).count()
    };
}