// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "Utils.h"
#include "integration/common/Diagnostic.h"

#include <iostream>
#include <filesystem>
#include <stdio.h>
#include <stdlib.h>

void printToken(CSTToken *token) {
    std::cout << " - [" << token->type_string() << "]" << "(" << token->start().representation() << ")";
}

void printTokens(const std::vector<std::unique_ptr<CSTToken>> &lexed) {
    for (const auto &item: lexed) {
        printToken(item.get());
        std::cout << std::endl;
    }
}

void printTokens(const std::vector<std::unique_ptr<CSTToken>> &lexed, const std::unordered_map<unsigned int, unsigned int> &linked) {
    int i = 0;
    while(i < lexed.size()) {
        auto found = linked.find(i);
        auto token = found == linked.end() ? lexed[i].get() : lexed[found->second].get();
        printToken(token);
        std::cout << std::endl;
        i++;
    }
}

std::string resolve_rel_child_path_str(const std::string& root_path, const std::string& file_path) {
    return (((std::filesystem::path) root_path) / ((std::filesystem::path) file_path)).string();
}

std::string resolve_parent_path(const std::string& root_path) {
    return ((std::filesystem::path) root_path).parent_path().string();
}

std::string resolve_non_canon_parent_path(const std::string& root_path, const std::string& file_path) {
    return (((std::filesystem::path) root_path).parent_path() / ((std::filesystem::path) file_path)).string();
}

std::string resolve_sibling(const std::string& rel_to, const std::string& sibling) {
    return (((std::filesystem::path) rel_to).parent_path() / ((std::filesystem::path) sibling)).string();
}

std::string resolve_rel_parent_path_str(const std::string& root_path, const std::string& file_path) {
    try {
        return std::filesystem::canonical(((std::filesystem::path) root_path).parent_path() / ((std::filesystem::path) file_path)).string();
    } catch (std::filesystem::filesystem_error& e) {
        return "";
    }
}

std::string resources_path_rel_to_exe(const std::string& exe_path) {
    auto res = resolve_rel_parent_path_str(exe_path, "resources");
#ifdef DEBUG
    if(res.empty()) {
        res = resolve_rel_parent_path_str(exe_path, "../lib/include");
    }
#endif
    return res;
}

std::string absolute_path(const std::string& relative) {
    return std::filesystem::absolute(relative).string();
}

#ifdef _WIN32
#include <windows.h>
int launch_executable(char* path, bool same_window) {
    if (same_window) {
        // Launch in the same window
        return system(path);
    } else {
        // Launch in a new window
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        if (!CreateProcess(NULL, path, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
            fprintf(stderr, "CreateProcess failed (%d).\n", GetLastError());
            return 1;
        }

        // Wait until child process exits.
        WaitForSingleObject(pi.hProcess, INFINITE);

        // Close process and thread handles.
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return 0;
    }
}
#else
#include <unistd.h>
int launch_executable(char* path, bool same_window) {
    if (same_window) {
        // Launch in the same window
        return system(path);
    } else {
        // Launch in a new window
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            execl(path, path, NULL);
            std::cerr << "execl failed for executable path " << path << std::endl;
            return 1;
        } else if (pid < 0) {
            // Fork failed
            std::cerr << "Process fork failed for executable path " << path << std::endl;
            return 1;
        } else {
            // Parent process
            wait(NULL); // Wait for child process to complete
            return 0;
        }
    }
}
#endif