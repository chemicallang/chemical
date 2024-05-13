// Copyright (c) Qinetik 2024.

#include <string>
#include "utils/functionalfwd.h"

struct BenchmarkResults {

    long long nanos;
    long long micros;
    long long millis;
    long long seconds;

    std::string representation();

};

BenchmarkResults benchmark(const std::function<void()>& fn);