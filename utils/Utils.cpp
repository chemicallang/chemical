// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "Environment.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

#if defined(_WIN32)
#include <Shlwapi.h>
#include <io.h>
#endif

#ifdef __APPLE__
#include <libgen.h>
    #include <limits.h>
    #include <mach-o/dyld.h>
    #include <unistd.h>
#endif

#ifdef __linux__
#include <limits.h>
    #include <libgen.h>
    #include <unistd.h>

    #if defined(__sun)
        #define PROC_SELF_EXE "/proc/self/path/a.out"
    #else
        #define PROC_SELF_EXE "/proc/self/exe"
    #endif

#endif

#if defined(_WIN32)
std::string getExecutablePath() {
    char rawPathName[MAX_PATH];
    GetModuleFileNameA(NULL, rawPathName, MAX_PATH);
    return std::string(rawPathName);
}
#endif

#ifdef __linux__
std::string getExecutablePath() {
   char rawPathName[PATH_MAX];
   realpath(PROC_SELF_EXE, rawPathName);
   return  std::string(rawPathName);
}
#endif

#ifdef __APPLE__
std::string getExecutablePath() {
    char rawPathName[PATH_MAX];
    char realPathName[PATH_MAX];
    uint32_t rawPathSize = (uint32_t)sizeof(rawPathName);
    if(!_NSGetExecutablePath(rawPathName, &rawPathSize)) {
        realpath(rawPathName, realPathName);
    }
    return  std::string(realPathName);
}
#endif


std::string resolve_rel_child_path_str(const std::string_view& root_path, const std::string_view& file_path) {
    return (((std::filesystem::path) root_path) / ((std::filesystem::path) file_path)).string();
}
std::string resolve_parent_path(const std::string_view& root_path) {
    return ((std::filesystem::path) root_path).parent_path().string();
}

std::string resolve_non_canon_parent_path(const std::string_view& root_path, const std::string_view& file_path) {
    return (((std::filesystem::path) root_path).parent_path() / ((std::filesystem::path) file_path)).string();
}

std::string resolve_sibling(const std::string_view& rel_to, const std::string_view& sibling) {
    return (((std::filesystem::path) rel_to).parent_path() / ((std::filesystem::path) sibling)).string();
}

std::string resolve_rel_parent_path_str(const std::string& root_path, const std::string& file_path) {
    // build the path we want to canonicalize
    auto p = std::filesystem::path(root_path).parent_path() / std::filesystem::path(file_path);
    std::error_code ec;
    auto result = std::filesystem::canonical(p, ec); // uses non-throwing overload
    if (ec) {
        // canonical failed (e.g. path doesn't exist, permission denied)
        return "";
    }
    return result.string();
}

std::string resolve_rel_parent_path_str(const std::string_view& root_path, const std::string_view& file_path) {
    auto p = ((std::filesystem::path) root_path).parent_path() / ((std::filesystem::path) file_path);
    std::error_code ec;
    auto result = std::filesystem::canonical(p, ec);
    if(ec) {
        return "";
    }
    return result.string();
}

std::string resources_path_rel_to_exe(const std::string_view& exe_path) {
    auto res = resolve_rel_parent_path_str(exe_path, "resources");
#ifdef DEBUG
    if(res.empty()) {
        res = resolve_rel_parent_path_str(exe_path, "../lib/include");
    }
#endif
    return res;
}

std::string canonical(const std::string_view& path) {
    std::error_code ec;
    auto result = std::filesystem::canonical(((std::filesystem::path) path), ec);
    if(ec) {
        return "";
    }
    return result.string();
}

std::string absolute_path(const std::string_view& relative) {
    return std::filesystem::absolute(relative).string();
}

std::string compiler_exe_path_relative_to_tcc(const std::string_view& exe_path) {
    return resolve_sibling(exe_path, chemical_exe_PATH_name());
}

int launch_exe_in_same_window(char* path) {
    return system(path);
}

// Required headers (put near top of file)
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cerrno>

#ifdef _WIN32
#include <windows.h>
#include <process.h>   // _beginthreadex
#else
#include <spawn.h>
  #include <sys/types.h>
  #include <sys/wait.h>
  #include <unistd.h>
  #include <poll.h>
  #include <fcntl.h>
  extern char **environ;
#endif

// Return: >=0 = exit code (if waited), <0 = failure code
int launch_process_and_wait(
        const char* exe_path,
        char* argv[],        // argv style, null-terminated
        size_t argi,         // number of args (or you can ignore and rely on argv null)
        bool waitForExit = true)
{
#ifdef _WIN32
    // Build commandline from argv into a writable stack buffer.
    // (CreateProcessA will be given exe_path as application name and cmdline as arguments.)
    char cmdline[32 * 1024];
    size_t pos = 0;
    for (size_t i = 0; argv[i] != nullptr; ++i) {
        const char* a = argv[i];
        bool need_quote = false;
        for (const char* p = a; *p; ++p) {
            if (*p == ' ' || *p == '\t') { need_quote = true; break; }
        }
        if (need_quote) {
            if (pos + 1 >= sizeof(cmdline)) return -4;
            cmdline[pos++] = '"';
            for (const char* p = a; *p; ++p) {
                if (pos + 2 >= sizeof(cmdline)) return -5;
                if (*p == '"') { cmdline[pos++] = '\\'; cmdline[pos++] = '"'; }
                else cmdline[pos++] = *p;
            }
            if (pos + 1 >= sizeof(cmdline)) return -6;
            cmdline[pos++] = '"';
        } else {
            for (const char* p = a; *p; ++p) {
                if (pos + 1 >= sizeof(cmdline)) return -7;
                cmdline[pos++] = *p;
            }
        }
        // separator
        if (argv[i + 1] != nullptr) {
            if (pos + 1 >= sizeof(cmdline)) return -8;
            cmdline[pos++] = ' ';
        }
    }
    if (pos >= sizeof(cmdline)) return -9;
    cmdline[pos] = '\0';

    // Create pipes for stdout and stderr
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = nullptr;
    sa.bInheritHandle = TRUE;

    HANDLE childStdOutRead = nullptr, childStdOutWrite = nullptr;
    HANDLE childStdErrRead = nullptr, childStdErrWrite = nullptr;

    if (!CreatePipe(&childStdOutRead, &childStdOutWrite, &sa, 0)) {
        return -110 - static_cast<int>(GetLastError());
    }
    if (!SetHandleInformation(childStdOutRead, HANDLE_FLAG_INHERIT, 0)) { // parent read should NOT be inheritable
        // not fatal, continue
    }

    if (!CreatePipe(&childStdErrRead, &childStdErrWrite, &sa, 0)) {
        CloseHandle(childStdOutRead); CloseHandle(childStdOutWrite);
        return -111 - static_cast<int>(GetLastError());
    }
    if (!SetHandleInformation(childStdErrRead, HANDLE_FLAG_INHERIT, 0)) {
        // not fatal
    }

    // Prepare STARTUPINFO to point child's stdout/stderr to pipe write handles.
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = childStdOutWrite;
    si.hStdError  = childStdErrWrite;
    ZeroMemory(&pi, sizeof(pi));

    // Create the process with inherited handles (so child can use the write pipe ends).
    BOOL ok = CreateProcessA(
            exe_path,      // lpApplicationName
            cmdline,       // lpCommandLine (writable)
            nullptr, nullptr,
            TRUE,          // bInheritHandles -> child inherits write handles
            0,             // creation flags (don't hide window)
            nullptr,       // environment
            nullptr,       // current directory
            &si,
            &pi
    );

    // Close the pipe write handles in the parent (parent only reads)
    // IMPORTANT: close them before reading to allow EOF when child exits.
    CloseHandle(childStdOutWrite);
    CloseHandle(childStdErrWrite);

    if (!ok) {
        CloseHandle(childStdOutRead);
        CloseHandle(childStdErrRead);
        return -100 - static_cast<int>(GetLastError());
    }

    // Reader thread function: reads from pipe and writes to parent's console handle
    struct ReaderParam { HANDLE from; HANDLE to; };
    auto reader_fn = [](void* pv) -> unsigned {
        ReaderParam *p = reinterpret_cast<ReaderParam*>(pv);
        const DWORD BUF_SZ = 4096;
        char buffer[BUF_SZ];
        DWORD readBytes = 0;
        for (;;) {
            BOOL r = ReadFile(p->from, buffer, BUF_SZ, &readBytes, nullptr);
            if (!r || readBytes == 0) break;
            DWORD written = 0;
            // Write to parent's std handle (may be console or redirected file)
            WriteFile(p->to, buffer, readBytes, &written, nullptr);
        }
        return 0;
    };

    // create two thread contexts on stack
    ReaderParam outParam{ childStdOutRead, GetStdHandle(STD_OUTPUT_HANDLE) };
    ReaderParam errParam{ childStdErrRead, GetStdHandle(STD_ERROR_HANDLE) };

    uintptr_t outThread = _beginthreadex(nullptr, 0, (unsigned (__stdcall *)(void *))reader_fn, &outParam, 0, nullptr);
    uintptr_t errThread = _beginthreadex(nullptr, 0, (unsigned (__stdcall *)(void *))reader_fn, &errParam, 0, nullptr);

    int exitCode = 0;
    if (waitForExit) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD code = 0;
        GetExitCodeProcess(pi.hProcess, &code);
        exitCode = static_cast<int>(code);
    } else {
        exitCode = 0; // or return pi.dwProcessId
    }

    // Wait for reader threads to finish reading remaining output
    if (outThread) {
        WaitForSingleObject(reinterpret_cast<HANDLE>(outThread), INFINITE);
        CloseHandle(reinterpret_cast<HANDLE>(outThread));
    }
    if (errThread) {
        WaitForSingleObject(reinterpret_cast<HANDLE>(errThread), INFINITE);
        CloseHandle(reinterpret_cast<HANDLE>(errThread));
    }

    // Close remaining handles
    CloseHandle(childStdOutRead);
    CloseHandle(childStdErrRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode;

#else
    // POSIX: create pipes, fork, dup2 child's stdout/stderr to pipes, parent reads and writes to parent's stdout/stderr.
    int outpipe[2];
    int errpipe[2];
    if (pipe(outpipe) != 0) return -210 - errno;
    if (pipe(errpipe) != 0) { close(outpipe[0]); close(outpipe[1]); return -211 - errno; }

    pid_t pid = fork();
    if (pid < 0) {
        // fork failed
        close(outpipe[0]); close(outpipe[1]); close(errpipe[0]); close(errpipe[1]);
        return -212 - errno;
    }

    if (pid == 0) {
        // Child
        // Redirect stdout and stderr to the write ends of the pipes
        // Close read ends first
        close(outpipe[0]);
        close(errpipe[0]);
        if (dup2(outpipe[1], STDOUT_FILENO) == -1) _exit(127);
        if (dup2(errpipe[1], STDERR_FILENO) == -1) _exit(127);
        // Close the original write fds after dup2
        close(outpipe[1]);
        close(errpipe[1]);

        // Execute
        execvp(argv[0], argv);
        // If execvp fails:
        _exit(127);
    }

    // Parent
    // Close write ends, keep read ends
    close(outpipe[1]);
    close(errpipe[1]);

    // Make read ends non-blocking? We'll use poll to avoid blocking on one fd only.
    // (Using blocking reads with poll is OK; we leave them blocking and use poll)
    struct pollfd fds[2];
    fds[0].fd = outpipe[0];
    fds[0].events = POLLIN;
    fds[1].fd = errpipe[0];
    fds[1].events = POLLIN;

    const size_t BUF_SZ = 4096;
    char buffer[BUF_SZ];
    bool out_open = true, err_open = true;
    while (out_open || err_open) {
        int nf = poll(fds, 2, -1);
        if (nf < 0) {
            if (errno == EINTR) continue;
            break;
        }
        // stdout pipe
        if (out_open && (fds[0].revents & (POLLIN | POLLHUP))) {
            ssize_t r = read(outpipe[0], buffer, BUF_SZ);
            if (r > 0) {
                ssize_t w = write(STDOUT_FILENO, buffer, static_cast<size_t>(r));
                (void)w;
            } else {
                out_open = false;
                fds[0].events = 0;
            }
        }
        // stderr pipe
        if (err_open && (fds[1].revents & (POLLIN | POLLHUP))) {
            ssize_t r = read(errpipe[0], buffer, BUF_SZ);
            if (r > 0) {
                ssize_t w = write(STDERR_FILENO, buffer, static_cast<size_t>(r));
                (void)w;
            } else {
                err_open = false;
                fds[1].events = 0;
            }
        }
    }

    // Close read ends
    close(outpipe[0]);
    close(errpipe[0]);

    if (!waitForExit) {
        // Return child's pid if non-blocking desired
        return static_cast<int>(pid);
    }

    // Wait for child to exit and return status
    int status = 0;
    pid_t r;
    do { r = waitpid(pid, &status, 0); } while (r == -1 && errno == EINTR);
    if (r == -1) return -300 - errno;

    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return -301;
#endif
}

#ifdef _WIN32
int launch_exe_in_sep_window(char* cmdline) {
    // Launch in a new window
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcess(NULL, cmdline, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
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
#else
#include <unistd.h>
#include <sys/wait.h>
int launch_exe_in_sep_window(char* path) {
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

std::string cmd_read_out(const std::string& command) {
    std::string result;
    FILE* pipe = nullptr;

#ifdef _WIN32
    // Windows-specific code
    // Use _popen for Windows
    pipe = _popen(command.c_str(), "r");
#else
    // POSIX-specific code
    // Use popen for UNIX-like systems
    pipe = popen(command.c_str(), "r");
#endif

    if (!pipe) {
        return ""; // Return empty string on failure
    }

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

#ifdef _WIN32
    // Windows-specific code
    _pclose(pipe);
#else
    // POSIX-specific code
    pclose(pipe);
#endif

    // Trim any extra whitespace or newlines
    result.erase(result.find_last_not_of("\r\n") + 1);

    return result;
}

bool add_to_PATH(const std::string& new_path, bool for_system) {
#ifdef _WIN32
    LPCSTR key_path = for_system ?
                      "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" :
                      "Environment";
    HKEY key;
    HKEY hive = for_system ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
    LONG result = RegOpenKeyEx(hive, key_path, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &key);
    if (result != ERROR_SUCCESS) {
        std::cerr << "Failed to open registry key. Error code: " << result << std::endl;
        return false;
    }

    // Get the existing PATH value
    DWORD path_size;
    result = RegQueryValueEx(key, "PATH", nullptr, nullptr, nullptr, &path_size);
    if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) {
        std::cerr << "Failed to query registry value. Error code: " << result << std::endl;
        RegCloseKey(key);
        return false;
    }

    std::string old_path_value(path_size, '\0');
    if (result != ERROR_FILE_NOT_FOUND) {
        result = RegQueryValueEx(key, "PATH", nullptr, nullptr, reinterpret_cast<BYTE*>(&old_path_value[0]), &path_size);
        if (result != ERROR_SUCCESS) {
            std::cerr << "Failed to read registry value. Error code: " << result << std::endl;
            RegCloseKey(key);
            return false;
        }
    }

    // Remove trailing null characters
    old_path_value.erase(std::find(old_path_value.begin(), old_path_value.end(), '\0'), old_path_value.end());

    // Append the new path to the existing PATH
    std::string new_path_value = old_path_value + ";" + new_path;

    result = RegSetValueEx(key, "PATH", 0, REG_SZ, (BYTE*)new_path_value.c_str(), new_path_value.size() + 1);
    if (result != ERROR_SUCCESS) {
        std::cerr << "Failed to set registry value. Error code: " << result << std::endl;
        RegCloseKey(key);
        return false;
    }
    RegCloseKey(key);
    return true;
#else
    const std::string file_path = for_system ? "/etc/environment" : std::getenv("HOME") + std::string("/.bashrc");
    std::ifstream file_in(file_path);
    std::string content;
    bool path_found = false;

    if (file_in.is_open()) {
        std::string line;
        while (std::getline(file_in, line)) {
            if (line.find("export PATH=") != std::string::npos) {
                path_found = true;
                // Append the new path to the existing PATH line
                std::string old_path_value = line.substr(12);
                line = "export PATH=" + old_path_value + ":" + new_path;
            }
            content += line + "\n";
        }
        file_in.close();
    }

    if (!path_found) {
        // If PATH is not found, append it at the end
        content += "export PATH=$PATH:" + new_path + "\n";
    }

    std::ofstream file_out(file_path, std::ios_base::trunc);
    if (file_out.is_open()) {
        file_out << content;
        file_out.close();
        return true;
    } else {
        std::cerr << "Failed to open file for writing: " << file_path << std::endl;
        return false;
    }
#endif
}

std::string get_chemical_version_in_PATH() {
    auto output = cmd_read_out_stderr_to_null(chemical_exe_PATH_name() + " -v");
    if(output.empty()) {
        return "";
    } else {
        if(output.starts_with("Chemical v")) {
            return output.substr(10);
        } else {
            return "";
        }
    }
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

#ifdef _WIN32
std::string getUserHomeDirectory() {
    char path[MAX_PATH];
    if (GetEnvironmentVariableA("USERPROFILE", path, MAX_PATH)) {
        return std::string(path);
    }
    return "";
}
#else
std::string getUserHomeDirectory() {
    const char* home = std::getenv("HOME");
    return home ? std::string(home) : "";
}
#endif