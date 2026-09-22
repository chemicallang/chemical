// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <future>
#include <vector>

/**
 * joins every future in the given vector when this object goes out of scope
 *
 * why this exists : a task pushed to the thread pool keeps running until it finishes.
 * Destroying a std::future does NOT wait for it (only std::async does, and this pool
 * uses packaged tasks). So a function that returns early while some of its futures
 * are still pending leaves those tasks running against state the caller is about to
 * leave behind (the processor, the allocators and the caller's locals are often stack
 * objects). They then write into stack memory that has been reused by deeper calls,
 * which corrupts whatever now lives there and produces crashes far away from the real
 * cause. Holding this object in the spawning function makes it impossible to return
 * without joining every task that was pushed.
 */
template<typename T>
class JoinedTasks {

    std::vector<std::future<T>>& futures;

public:

    explicit JoinedTasks(std::vector<std::future<T>>& futures) : futures(futures) {

    }

    JoinedTasks(const JoinedTasks&) = delete;
    JoinedTasks& operator=(const JoinedTasks&) = delete;

    ~JoinedTasks() {
        for(auto& future : futures) {
            if(future.valid()) {
                future.get();
            }
        }
    }

};
