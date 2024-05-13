// Copyright (c) Qinetik 2024.

#include <string>

struct BenchmarkResults {

    std::uint64_t start_time; // Native representation of start time
    std::uint64_t elapsed_time; // Native representation of elapsed time

    void benchmark_begin();

    void benchmark_end();

    std::string representation();


};