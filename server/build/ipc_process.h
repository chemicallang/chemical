// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>
#include <vector>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
using PROCESS_HANDLE = HANDLE;
#else
#include <sys/types.h>
  using PROCESS_HANDLE = pid_t;
#endif

enum class ChildReason {
    SUCCESS,
    CRASH,
    TIMEOUT
};

struct ChildResult {
    ChildReason reason;
    int exitCode;
    std::string resultString;
};

std::string generate_shm_name();
bool launch_child_process(const std::vector<std::string>& argv, PROCESS_HANDLE& outHandle);
bool wait_for_child_and_read(PROCESS_HANDLE handle, const std::string& shmName,
                             int timeoutMs, ChildResult& outResult);
bool child_create_and_write_shm(const std::string& shmName, const std::string& payload);

#ifdef _WIN32
int wmain(int argc, wchar_t** argv);
#else
int child_main(int argc, char** argv);
#endif
