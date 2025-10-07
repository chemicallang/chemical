// Copyright (c) Chemical Language Foundation 2025.

#include "Benchmark.h"
#include <chrono>

std::string BenchmarkResults::representation() {

    auto nanos = end_time - start_time;;
    auto micros = nanos / 1000;
    auto millis = nanos / 1000000;
    auto seconds = nanos / 1000000000;

    std::string rep;
    rep.append("[nano:");
    rep.append(std::to_string(nanos));
    rep.append("] [micro:");
    rep.append(std::to_string(micros));
    rep.append("] [milli:");
    rep.append(std::to_string(millis));
    rep.append("] [sec:");
    rep.append(std::to_string(seconds));
    rep.append("]");
    return rep;

}

uint64_t BenchmarkResults::millis() {
    return ((end_time - start_time) / 1000000);
}

void BenchmarkResults::benchmark_begin() {
    start_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

void BenchmarkResults::benchmark_end() {
    end_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

void BenchmarkResults::plus(const BenchmarkResults& results) {
    start_time += results.start_time;
    end_time += results.end_time;
}