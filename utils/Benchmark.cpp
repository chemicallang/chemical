// Copyright (c) Qinetik 2024.

#include "Benchmark.h"
#include <chrono>

std::string BenchmarkResults::representation() {

    auto nanos = elapsed_time;
    auto micros = nanos / 1000;
    auto millis = nanos / 1000000;
    auto seconds = nanos / 1000000000;

    std::string rep;
    rep.append("[Nano:");
    rep.append(std::to_string(nanos));
    rep.append("] Micro:");
    rep.append(std::to_string(micros));
    rep.append("] [Milli:");
    rep.append(std::to_string(millis));
    rep.append("] [Sec:");
    rep.append(std::to_string(seconds));
    rep.append("]");
    return rep;

}

void BenchmarkResults::benchmark_begin() {
    start_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

void BenchmarkResults::benchmark_end() {
    std::uint64_t end_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    elapsed_time = end_time - start_time;
}