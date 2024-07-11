// Copyright (c) Qinetik 2024.

#pragma once

#include <string>

struct BenchmarkResults {

    std::uint64_t start_time; // Native representation of start time
    std::uint64_t end_time; // Native representation of elapsed time

    void benchmark_begin();

    void benchmark_end();

    void plus(const BenchmarkResults& results);

    std::string representation();


};