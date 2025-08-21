// Copyright (c) Chemical Language Foundation 2025.

#pragma once

int launch_exe_in_same_window(char* cmdline);

int launch_exe_in_sep_window(char* cmdline);

int launch_process_and_wait(
        const char* exe_path,
        char* argv[],                                   // e.g. options->outMode
        size_t argi,
        bool waitForExit = true
);

inline int launch_executable(char* path, bool same_window) {
    if(same_window) {
        return launch_exe_in_same_window(path);
    } else {
        return launch_exe_in_sep_window(path);
    }
}