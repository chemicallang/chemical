// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "Utils.h"
#include "integration/common/Diagnostic.h"
#include "Environment.h"

#include <iostream>
#include <filesystem>
#include <stdio.h>
#include <stdlib.h>

void printToken(CSTToken *token) {
    std::cout << " - [" << token->type_string() << "]" << "(" << token->start().representation() << ")";
}

void printTokens(const std::vector<CSTToken*> &lexed) {
    for (const auto &item: lexed) {
        printToken(item);
        std::cout << std::endl;
    }
}

void printTokens(const std::vector<CSTToken*> &lexed, const std::unordered_map<unsigned int, unsigned int> &linked) {
    int i = 0;
    while(i < lexed.size()) {
        auto found = linked.find(i);
        auto token = found == linked.end() ? lexed[i] : lexed[found->second];
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

std::string canonical_path(const std::string& path) {
    try {
        return std::filesystem::canonical(((std::filesystem::path) path)).string();
    } catch (std::filesystem::filesystem_error& e) {
        return "";
    }
};

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
#include <sys/wait.h>
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

bool set_environment_variable(const std::string& name, const std::string& value, bool for_system) {
#ifdef _WIN32
    LPCSTR key_path = for_system ?
                      "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" :
                      "Environment";
    HKEY key;
    HKEY hive = for_system ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
    LONG result = RegOpenKeyEx(hive, key_path, 0, KEY_SET_VALUE, &key);
    if (result != ERROR_SUCCESS) {
        // Handle error or request admin privileges here
        std::cerr << "Failed to open registry key. Error code: " << result << std::endl;
        return false;
    }
    result = RegSetValueEx(key, name.c_str(), 0, REG_SZ, (BYTE*)value.c_str(), value.size() + 1);
    if (result != ERROR_SUCCESS) {
        std::cerr << "Failed to set registry value. Error code: " << result << std::endl;
        return false;
    }
    RegCloseKey(key);
    return true;
#else
    const std::string file_path = for_system ? "/etc/environment" : std::getenv("HOME") + std::string("/.bashrc");
    std::ofstream file(file_path, std::ios_base::app);
    if (file.is_open()) {
        file << "export " << name << "=" << value << std::endl;
        file.close();
        return true;
    } else {
        return false;
    }
#endif
}

#ifdef _WIN32

bool isAdmin() {
    BOOL isElevated = FALSE;
    HANDLE token = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elevation;
        DWORD size;
        if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
            isElevated = elevation.TokenIsElevated;
        }
    }
    if (token) {
        CloseHandle(token);
    }
    return isElevated;
}

bool relaunchAsAdmin() {
    char szPath[MAX_PATH];
    if (GetModuleFileName(NULL, szPath, MAX_PATH)) {
        SHELLEXECUTEINFO sei = { sizeof(sei) };
        sei.lpVerb = "runas";
        sei.lpFile = szPath;
        sei.hwnd = NULL;
        sei.lpParameters = GetCommandLine();
        sei.nShow = SW_NORMAL;
        if (!ShellExecuteEx(&sei)) {
            std::cerr << "Failed to elevate privileges. Error code: " << GetLastError() << std::endl;
            return false;
        }
        exit(0); // Exit current process if relaunching
        return true;
    } else {
        return false;
    }
}

#else

bool isSudo() {
     // Check if the user is root
    return (geteuid() == 0);
}

bool requestSudo() {
    // Try executing a harmless command with sudo to check if sudo access is available
//    int result = system("sudo -n true 2>/dev/null");
//    if (result == 0) {
//        return true; // User has sudo privileges without password prompt
//    }
//    std::cerr << "Sudo is required to perform this operation. Attempting to request sudo access..." << std::endl;
    return system("sudo -v") == 0;
}

#endif