// Copyright (c) Chemical Language Foundation 2025.

#include "ipc_process.h"

#include <random>
#include <sstream>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <objbase.h>
#else
#include <sys/mman.h>
  #include <sys/stat.h>
  #include <fcntl.h>
  #include <errno.h>
  #include <unistd.h>
  #include <signal.h>
  #include <sys/wait.h>
  #include <cstdio>
  #include <cstdlib>
#endif

//──────────────────────────────────────────────────────────────────────────
// generate_shm_name()
//──────────────────────────────────────────────────────────────────────────
std::string generate_shm_name() {
    std::ostringstream oss;
#ifdef _WIN32
    DWORD pid = GetCurrentProcessId();
#else
    pid_t pid = getpid();
#endif
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);
    uint32_t rnd = dist(gen);

    oss << "ipcshm_" << pid << "_"
        << std::hex << std::setw(8) << std::setfill('0') << rnd;
    return oss.str();
}

//──────────────────────────────────────────────────────────────────────────
// launch_child_process()
//──────────────────────────────────────────────────────────────────────────
#ifdef _WIN32

bool launch_child_process(const std::vector<std::string>& argv, PROCESS_HANDLE& outHandle) {
    if (argv.empty()) return false;

    // Build a wide‐string command line:
    std::wstring cmdLine;
    {
        int wlen = MultiByteToWideChar(CP_UTF8, 0, argv[0].c_str(), -1, nullptr, 0);
        if (wlen == 0) return false;
        std::wstring wexe(wlen, 0);
        MultiByteToWideChar(CP_UTF8, 0, argv[0].c_str(), -1, &wexe[0], wlen);
        if (!wexe.empty() && wexe.back() == L'\0') wexe.pop_back();
        if (wexe.find(L' ') != std::wstring::npos) {
            cmdLine += L"\"";
            cmdLine += wexe;
            cmdLine += L"\"";
        } else {
            cmdLine += wexe;
        }
    }
    for (size_t i = 1; i < argv.size(); i++) {
        cmdLine += L" ";
        int wlen = MultiByteToWideChar(CP_UTF8, 0, argv[i].c_str(), -1, nullptr, 0);
        if (wlen == 0) return false;
        std::wstring warg(wlen, 0);
        MultiByteToWideChar(CP_UTF8, 0, argv[i].c_str(), -1, &warg[0], wlen);
        if (!warg.empty() && warg.back() == L'\0') warg.pop_back();
        if (warg.find(L' ') != std::wstring::npos) {
            cmdLine += L"\"";
            cmdLine += warg;
            cmdLine += L"\"";
        } else {
            cmdLine += warg;
        }
    }

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // CreateProcess wants a writable LPWSTR:
    std::vector<wchar_t> cmdBuf(cmdLine.c_str(), cmdLine.c_str() + (cmdLine.size() + 1));
    BOOL ok = CreateProcessW(
            nullptr,
            cmdBuf.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &si,
            &pi
    );
    if (!ok) {
        return false;
    }
    CloseHandle(pi.hThread);
    outHandle = pi.hProcess;
    return true;
}

#else // POSIX

bool launch_child_process(const std::vector<std::string>& argv, PROCESS_HANDLE& outHandle) {
    if (argv.empty()) return false;

    pid_t pid = fork();
    if (pid < 0) {
        return false;
    }
    if (pid == 0) {
        // child:
        size_t n = argv.size();
        char** cargv = new char*[n + 1];
        for (size_t i = 0; i < n; i++) {
            cargv[i] = const_cast<char*>(argv[i].c_str());
        }
        cargv[n] = nullptr;
        execvp(cargv[0], cargv);
        _exit(127);
    }
    // parent:
    outHandle = pid;
    return true;
}

#endif // launch_child_process



//──────────────────────────────────────────────────────────────────────────
// wait_for_child_and_read()
//──────────────────────────────────────────────────────────────────────────
#ifdef _WIN32

static bool read_from_shared_memory_windows(const std::string& shmName, ChildResult& out) {
    int wlen = MultiByteToWideChar(CP_UTF8, 0, shmName.c_str(), -1, nullptr, 0);
    if (wlen == 0) return false;
    std::wstring wName(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, shmName.c_str(), -1, &wName[0], wlen);
    if (!wName.empty() && wName.back() == L'\0') wName.pop_back();

    HANDLE hMap = OpenFileMappingW(FILE_MAP_READ, FALSE, wName.c_str());
    if (!hMap) return false;

    SIZE_T headerSize = sizeof(size_t);
    LPVOID headerPtr = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, headerSize);
    if (!headerPtr) {
        CloseHandle(hMap);
        return false;
    }
    size_t dataLen = 0;
    memcpy(&dataLen, headerPtr, sizeof(size_t));
    UnmapViewOfFile(headerPtr);

    SIZE_T totalSize = sizeof(size_t) + dataLen;
    LPVOID fullPtr = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, totalSize);
    if (!fullPtr) {
        CloseHandle(hMap);
        return false;
    }
    char* bytePtr = static_cast<char*>(fullPtr) + sizeof(size_t);
    out.resultString.assign(bytePtr, dataLen);

    UnmapViewOfFile(fullPtr);
    CloseHandle(hMap);
    return true;
}

bool wait_for_child_and_read(PROCESS_HANDLE handle,
                             const std::string& shmName,
                             int timeoutMs,
                             ChildResult& outResult)
{
    DWORD wait = WaitForSingleObject(handle, (DWORD)timeoutMs);
    if (wait == WAIT_TIMEOUT) {
        TerminateProcess(handle, 1);
        CloseHandle(handle);

        outResult.reason = ChildReason::TIMEOUT;
        outResult.exitCode = -1;
        ChildResult tmp;
        if (read_from_shared_memory_windows(shmName, tmp)) {
            outResult.resultString = tmp.resultString;
        }
        return true;
    }

    DWORD exitCode = 0;
    if (!GetExitCodeProcess(handle, &exitCode)) {
        CloseHandle(handle);
        return false;
    }
    CloseHandle(handle);

    if (exitCode == 0) {
        outResult.reason = ChildReason::SUCCESS;
        outResult.exitCode = 0;
    } else {
        outResult.reason = ChildReason::CRASH;
        outResult.exitCode = (int)exitCode;
    }
    read_from_shared_memory_windows(shmName, outResult);
    return true;
}

#else // POSIX

static bool read_from_shared_memory_posix(const std::string& shmName, ChildResult& out) {
    std::string posixName = shmName;
    if (posixName.empty() || posixName[0] != '/') {
        posixName = "/" + posixName;
    }
    int fd = shm_open(posixName.c_str(), O_RDONLY, 0);
    if (fd < 0) return false;

    size_t headerSize = sizeof(size_t);
    void* hdr = mmap(nullptr, headerSize, PROT_READ, MAP_SHARED, fd, 0);
    if (hdr == MAP_FAILED) {
        close(fd);
        return false;
    }
    size_t dataLen = 0;
    memcpy(&dataLen, hdr, sizeof(size_t));
    munmap(hdr, headerSize);

    size_t totalSize = headerSize + dataLen;
    void* full = mmap(nullptr, totalSize, PROT_READ, MAP_SHARED, fd, 0);
    if (full == MAP_FAILED) {
        close(fd);
        return false;
    }
    char* cptr = static_cast<char*>(full) + headerSize;
    out.resultString.assign(cptr, dataLen);

    munmap(full, totalSize);
    close(fd);
    return true;
}

bool wait_for_child_and_read(PROCESS_HANDLE handle,
                             const std::string& shmName,
                             int timeoutMs,
                             ChildResult& outResult)
{
    int status = 0;
    auto start = std::chrono::steady_clock::now();
    const int checkIntervalMs = 50;

    while (true) {
        pid_t ret = waitpid(handle, &status, WNOHANG);
        if (ret == -1) {
            return false;
        }
        if (ret == handle) {
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                outResult.reason = ChildReason::SUCCESS;
                outResult.exitCode = 0;
            } else {
                outResult.reason = ChildReason::CRASH;
                if (WIFEXITED(status)) {
                    outResult.exitCode = WEXITSTATUS(status);
                } else if (WIFSIGNALED(status)) {
                    outResult.exitCode = 128 + WTERMSIG(status);
                } else {
                    outResult.exitCode = -1;
                }
            }
            read_from_shared_memory_posix(shmName, outResult);
            return true;
        }
        auto now = std::chrono::steady_clock::now();
        int elapsed = (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        if (elapsed >= timeoutMs) {
            kill(handle, SIGKILL);
            waitpid(handle, &status, 0);

            outResult.reason = ChildReason::TIMEOUT;
            outResult.exitCode = -1;
            read_from_shared_memory_posix(shmName, outResult);
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(checkIntervalMs));
    }
}

#endif // wait_for_child_and_read



//──────────────────────────────────────────────────────────────────────────
// child_create_and_write_shm()
//──────────────────────────────────────────────────────────────────────────
bool child_create_and_write_shm(const std::string& shmName, const std::string& payload) {
    size_t payloadSize = payload.size();
    size_t totalSize = sizeof(size_t) + payloadSize;

#ifdef _WIN32
    int wlen = MultiByteToWideChar(CP_UTF8, 0, shmName.c_str(), -1, nullptr, 0);
    if (wlen == 0) return false;
    std::wstring wName(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, shmName.c_str(), -1, &wName[0], wlen);
    if (!wName.empty() && wName.back() == L'\0') wName.pop_back();

    HANDLE hMap = CreateFileMappingW(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            (DWORD)totalSize,
            wName.c_str()
    );
    if (!hMap) return false;

    LPVOID base = MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, totalSize);
    if (!base) {
        CloseHandle(hMap);
        return false;
    }
    memcpy(base, &payloadSize, sizeof(size_t));
    memcpy(static_cast<char*>(base) + sizeof(size_t), payload.data(), payloadSize);

    UnmapViewOfFile(base);
    CloseHandle(hMap);
    return true;

#else
    std::string posixName = shmName;
    if (posixName.empty() || posixName[0] != '/') {
        posixName = "/" + posixName;
    }
    int fd = shm_open(posixName.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd < 0) {
        shm_unlink(posixName.c_str());
        fd = shm_open(posixName.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
        if (fd < 0) return false;
    }
    if (ftruncate(fd, totalSize) != 0) {
        close(fd);
        shm_unlink(posixName.c_str());
        return false;
    }
    void* base = mmap(nullptr, totalSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (base == MAP_FAILED) {
        close(fd);
        shm_unlink(posixName.c_str());
        return false;
    }
    memcpy(base, &payloadSize, sizeof(size_t));
    memcpy(static_cast<char*>(base) + sizeof(size_t), payload.data(), payloadSize);

    munmap(base, totalSize);
    close(fd);
    return true;
#endif
}