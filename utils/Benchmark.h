// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>
#include <cinttypes>

struct BenchmarkResults {

    std::uint64_t start_time; // Native representation of start time
    std::uint64_t end_time; // Native representation of elapsed time

    uint64_t millis();

    void benchmark_begin();

    void benchmark_end();

    void plus(const BenchmarkResults& results);

    std::string representation();


};