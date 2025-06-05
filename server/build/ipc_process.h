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

///
/// A small struct to hold the result from the child.
///
enum class ChildReason {
    SUCCESS,
    CRASH,
    TIMEOUT
};

struct ChildResult {
    ChildReason reason;
    int        exitCode;      // On Windows: GetExitCodeProcess; On POSIX: WEXITSTATUS or 128+signal
    std::string payload;      // The string the child wrote into shared memory
};

void make_unique_names(
    std::string &shmName,
    std::string &evtChildDone,
    std::string &evtParentAck
);

#ifdef _WIN32

std::wstring utf8_to_wide(const std::string &s);

bool parent_read_and_cleanup_windows(
        const std::string &shmName,
        const std::string &evtParentAck,
        ChildResult &out
);

bool launch_child_process(const std::vector<std::string>& argv, PROCESS_HANDLE& outHandle);

int child_create_and_write_shm(const std::string& shmName, const std::string& evtChild, const std::string& evtParent, const std::string& payload);

#else

bool parent_read_shared_memory(const std::string &shmName, ChildResult &out);

bool launch_child_process(const std::vector<std::string>& argv, pid_t &outPid);

int child_create_and_write_shm(const std::string& shmName, const std::string& evtChild, const std::string& evtParent, const std::string& payload);

#endif