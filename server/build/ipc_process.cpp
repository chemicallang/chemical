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
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <unistd.h>
    #include <signal.h>
    #include <fcntl.h>
    #include <semaphore.h>
    #include <cstring>
    #include <ctime>
    #include <pthread.h>
#endif

///
/// Generate a “reasonably unique” suffix for naming.
/// Combines PID + 32-bit random hex, e.g. "_12345_ab12cd34"
///
static std::string generate_suffix() {
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
    oss << "_" << pid << "_" << std::hex << std::setw(8) << std::setfill('0') << rnd;
    return oss.str();
}

///
/// Constructs unique object names (no leading slash for POSIX yet).
///   shmName:       name of the shared‐memory region
///   evtChildDone:  name of the “child signals parent” sync object
///   evtParentAck:  name of the “parent signals child” sync object
///
void make_unique_names(std::string &shmName,
                              std::string &evtChildDone,
                              std::string &evtParentAck)
{
    std::string suffix = generate_suffix();
    shmName       = "ipcshm"       + suffix;
    evtChildDone  = "ipcevt_child" + suffix;
    evtParentAck  = "ipcevt_parent" + suffix;
}

//──────────────────────────────────────────────────────────────────────────
// launch_child_process()
//──────────────────────────────────────────────────────────────────────────
#ifdef _WIN32

std::wstring utf8_to_wide(const std::string &s) {
    int wlen = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (wlen == 0) return L"";
    std::wstring w(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], wlen);
    // Remove trailing null if any:
    if (!w.empty() && w.back() == L'\0') w.pop_back();
    return w;
}

// Launch the child (using CreateProcessW). argv[0] = path to child executable,
// argv[1..] = arguments (UTF-8). Returns a HANDLE in outProcess.
bool launch_child_process(const std::vector<std::string> &argv, HANDLE &outProcess)
{
    if (argv.empty()) return false;

    // Build a wide-char command line string:
    std::wstring cmdLine;
    {
        // Convert argv[0] to UTF-16:
        std::wstring wexe = utf8_to_wide(argv[0]);
        if (wexe.find(L' ') != std::wstring::npos) {
            cmdLine += L"\"" + wexe + L"\"";
        } else {
            cmdLine += wexe;
        }
    }
    for (size_t i = 1; i < argv.size(); i++) {
        cmdLine += L" ";
        std::wstring warg = utf8_to_wide(argv[i]);
        if (warg.find(L' ') != std::wstring::npos) {
            cmdLine += L"\"" + warg + L"\"";
        } else {
            cmdLine += warg;
        }
    }

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // We need a non-const LPWSTR for CreateProcessW:
    std::vector<wchar_t> cmdBuf(cmdLine.c_str(), cmdLine.c_str() + cmdLine.size() + 1);

    BOOL ok = CreateProcessW(
            nullptr,              // lpApplicationName
            cmdBuf.data(),        // lpCommandLine
            nullptr,              // lpProcessAttributes
            nullptr,              // lpThreadAttributes
            FALSE,                // bInheritHandles
            0,                    // dwCreationFlags
            nullptr,              // lpEnvironment
            nullptr,              // lpCurrentDirectory
            &si,
            &pi
    );
    if (!ok) {
        DWORD e = GetLastError();
        std::cerr << "[Parent] CreateProcessW failed (GLE=" << e << ")" << std::endl;
        return false;
    }
    CloseHandle(pi.hThread);    // we don't need the thread handle
    outProcess = pi.hProcess;   // parent will wait on this or kill it
    return true;
}

///
/// Parent does two things on Windows:
///   1) CreateEvent(evParentAck)   // so that child’s OpenEvent won’t fail
///   2) Launch childBuild.exe with args: ( buildArg, shmName, evtChildDone, evtParentAck )
///   3) CreateEvent(evChildDone)   // then WaitForSingleObject on evChildDone (with timeout)
///   4) If evChildDone signaled: OpenFileMapping(shmName), MapViewOfFile to read header+payload,
///      then OpenEvent(evParentAck) and SetEvent on it (to let child close its HANDLE)
///   5) If timeout: TerminateProcess(child), mark TIMEOUT, attempt to OpenFileMapping anyway
///   6) Close all handles, return a ChildResult
///

bool parent_read_and_cleanup_windows(
    const std::string &shmName,
    const std::string &evtParentAck,
    ChildResult &out
) {
    // 1) Open the file mapping the child created:
    std::wstring wShm = utf8_to_wide(shmName);
    HANDLE hMap = OpenFileMappingW(FILE_MAP_READ, FALSE, wShm.c_str());
    if (!hMap) {
        DWORD e = GetLastError();
        // If the child crashed before creating it, just return false
        std::cerr << "[Parent] OpenFileMappingW failed (GLE=" << e << ")\n";
        return false;
    }

    // 2) Map just sizeof(size_t) to get payload length:
    SIZE_T headerSize = sizeof(size_t);
    LPVOID hdrPtr = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, headerSize);
    if (!hdrPtr) {
        DWORD e = GetLastError();
        std::cerr << "[Parent] MapViewOfFile(header) failed (GLE=" << e << ")\n";
        CloseHandle(hMap);
        return false;
    }
    size_t dataLen = 0;
    memcpy(&dataLen, hdrPtr, sizeof(size_t));
    UnmapViewOfFile(hdrPtr);

    // 3) Map entire region of (headerSize + dataLen):
    SIZE_T totalSize = headerSize + dataLen;
    LPVOID fullPtr = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, totalSize);
    if (!fullPtr) {
        DWORD e = GetLastError();
        std::cerr << "[Parent] MapViewOfFile(full) failed (GLE=" << e << ")\n";
        CloseHandle(hMap);
        return false;
    }
    char *cbuf = reinterpret_cast<char*>(fullPtr) + sizeof(size_t);
    out.payload.assign(cbuf, dataLen);

    UnmapViewOfFile(fullPtr);
    CloseHandle(hMap);

    // 4) Signal parentAck so child knows it can close:
    std::wstring wAck = utf8_to_wide(evtParentAck);
    HANDLE hAckEvt = OpenEventW(EVENT_MODIFY_STATE, FALSE, wAck.c_str());
    if (!hAckEvt) {
        DWORD e = GetLastError();
        std::cerr << "[Parent] OpenEventW(parentAck) failed (GLE=" << e << ")\n";
        return false;
    }
    SetEvent(hAckEvt);
    CloseHandle(hAckEvt);

    return true;
}

int child_create_and_write_shm(const std::string& shmName, const std::string& evtChild, const std::string& evtParent, const std::string& payload) {

    // 3) CreateFileMapping of exactly (sizeof(size_t)+payload.size()):
    size_t payloadSize = payload.size();
    size_t totalSize   = sizeof(size_t) + payloadSize;

    // Convert shmName→UTF-16:
    std::wstring wShm = utf8_to_wide(shmName);
    HANDLE hMap = CreateFileMappingW(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            (DWORD)totalSize,
            wShm.c_str()
    );
    if (!hMap) {
        DWORD e = GetLastError();
        std::cerr << "[Child] CreateFileMappingW failed (GLE=" << e << ")\n";
        return 1;
    }

    // 4) MapViewOfFile & write header+payload:
    LPVOID base = MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, totalSize);
    if (!base) {
        DWORD e = GetLastError();
        std::cerr << "[Child] MapViewOfFile failed (GLE=" << e << ")\n";
        CloseHandle(hMap);
        return 1;
    }
    memcpy(base, &payloadSize, sizeof(size_t));
    memcpy(reinterpret_cast<char*>(base) + sizeof(size_t), payload.data(), payloadSize);
    UnmapViewOfFile(base);

    // 5) Create (or open) the “childDone” event and signal it:
    std::wstring wChildDone = utf8_to_wide(evtChild);
    HANDLE hChildEvt = CreateEventW(
            nullptr,   // no security
            FALSE,     // auto-reset
            FALSE,     // initially nonsignaled
            wChildDone.c_str()
    );
    if (!hChildEvt) {
        DWORD e = GetLastError();
        std::cerr << "[Child] CreateEventW(childDone) failed (GLE=" << e << ")\n";
        CloseHandle(hMap);
        return 1;
    }
    SetEvent(hChildEvt);

    // 6) Open “parentAck” event (parent created it before launching):
    std::wstring wParentAck = utf8_to_wide(evtParent);
    HANDLE hParentEvt = OpenEventW(
            SYNCHRONIZE,
            FALSE,
            wParentAck.c_str()
    );
    if (!hParentEvt) {
        DWORD e = GetLastError();
        std::cerr << "[Child] OpenEventW(parentAck) failed (GLE=" << e << ")\n";
        CloseHandle(hChildEvt);
        CloseHandle(hMap);
        return 1;
    }

    // 7) Wait for parentAck to be signaled:
    DWORD wait = WaitForSingleObject(hParentEvt, INFINITE);
    if (wait != WAIT_OBJECT_0) {
        DWORD e = GetLastError();
        std::cerr << "[Child] WaitForSingleObject(parentAck) failed (GLE=" << e << ")\n";
        // Even if it fails, proceed to cleanup:
    }

    // 8) Now close mapping handle and exit:
    CloseHandle(hParentEvt);
    CloseHandle(hChildEvt);
    CloseHandle(hMap);

    return 0;
}

#else

bool launch_child_process(const std::vector<std::string>& argv, pid_t &outPid) {
    if (argv.empty()) return false;
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "[Parent] fork() failed: " << strerror(errno) << "\n";
        return false;
    }
    if (pid == 0) {
        // In child: build a char*[] for execvp
        size_t n = argv.size();
        char **cargv = new char*[n+1];
        for (size_t i = 0; i < n; i++) {
            cargv[i] = const_cast<char*>(argv[i].c_str());
        }
        cargv[n] = nullptr;
        execvp(cargv[0], cargv);
        // If execvp returns, it failed:
        std::cerr << "[Child] execvp failed: " << strerror(errno) << "\n";
        _exit(127);
    }
    // In parent:
    outPid = pid;
    return true;
}

///
/// Attempt to open the shared memory and read header+payload.
/// Returns true if we got any data (payload may be empty).
///
bool parent_read_shared_memory(const std::string &shmName, ChildResult &out) {
    // POSIX: shared memory name must start with '/'
    std::string posixName = shmName;
    if (posixName.empty() || posixName[0] != '/')
        posixName = "/" + posixName;

    int fd = shm_open(posixName.c_str(), O_RDONLY, 0);
    if (fd < 0) {
        // No shared memory exists
        // std::cerr << "[Parent] shm_open failed: " << strerror(errno) << "\n";
        return false;
    }
    // 1) mmap the header to get length:
    size_t headerSize = sizeof(size_t);
    void *hdr = mmap(nullptr, headerSize, PROT_READ, MAP_SHARED, fd, 0);
    if (hdr == MAP_FAILED) {
        // std::cerr << "[Parent] mmap(header) failed: " << strerror(errno) << "\n";
        close(fd);
        return false;
    }
    size_t dataLen = 0;
    memcpy(&dataLen, hdr, sizeof(size_t));
    munmap(hdr, headerSize);

    // 2) mmap the full region
    size_t totalSize = headerSize + dataLen;
    void *full = mmap(nullptr, totalSize, PROT_READ, MAP_SHARED, fd, 0);
    if (full == MAP_FAILED) {
        // std::cerr << "[Parent] mmap(full) failed: " << strerror(errno) << "\n";
        close(fd);
        return false;
    }
    char *cbuf = reinterpret_cast<char*>(full) + sizeof(size_t);
    out.payload.assign(cbuf, dataLen);

    munmap(full, totalSize);
    close(fd);
    return true;
}

int child_create_and_write_shm(const std::string& shmName, const std::string& semChild, const std::string& semParent, const std::string& payload) {

    // 3) Create shared memory of exactly header+payload size:
    size_t payloadSize = payload.size();
    size_t totalSize   = sizeof(size_t) + payloadSize;

    // POSIX: shm_open requires a leading '/'
    std::string posixShm = shmName;
    if (posixShm.empty() || posixShm[0] != '/') posixShm = "/" + posixShm;

    // If it already exists for some reason, unlink first:
    shm_unlink(posixShm.c_str());
    int fd = shm_open(posixShm.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd < 0) {
        std::cerr << "[Child] shm_open failed: " << strerror(errno) << "\n";
        return 1;
    }
    if (ftruncate(fd, totalSize) != 0) {
        std::cerr << "[Child] ftruncate failed: " << strerror(errno) << "\n";
        close(fd);
        shm_unlink(posixShm.c_str());
        return 1;
    }
    void *base = mmap(nullptr, totalSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (base == MAP_FAILED) {
        std::cerr << "[Child] mmap failed: " << strerror(errno) << "\n";
        close(fd);
        shm_unlink(posixShm.c_str());
        return 1;
    }
    // Write header + payload:
    memcpy(base, &payloadSize, sizeof(size_t));
    memcpy(reinterpret_cast<char*>(base) + sizeof(size_t), payload.data(), payloadSize);
    munmap(base, totalSize);
    close(fd);
    // Do NOT shm_unlink yet: parent needs to open it

    // 4) Create (or open) “childDone” semaphore (initially 0) and post:
    std::string semChildName = semChild;
    if (semChildName.empty() || semChildName[0] != '/') semChildName = "/" + semChildName;
    sem_unlink(semChildName.c_str());
    sem_t *sChild = sem_open(semChildName.c_str(), O_CREAT | O_EXCL, 0600, 0);
    if (sChild == SEM_FAILED) {
        std::cerr << "[Child] sem_open(childDone) failed: " << strerror(errno) << "\n";
        return 1;
    }
    sem_post(sChild);

    // 5) Open “parentAck” semaphore (parent already created it):
    std::string semParentName = semParent;
    if (semParentName.empty() || semParentName[0] != '/') semParentName = "/" + semParentName;
    sem_t *sParent = sem_open(semParentName.c_str(), 0);
    if (sParent == SEM_FAILED) {
        std::cerr << "[Child] sem_open(parentAck) failed: " << strerror(errno) << "\n";
        sem_close(sChild);
        return 1;
    }

    // 6) Wait on parentAck:
    sem_wait(sParent);

    // 7) Cleanup:
    sem_close(sChild);
    sem_unlink(semChildName.c_str());

    sem_close(sParent);
    sem_unlink(semParentName.c_str());

    // Finally, unlink the shared memory so that the OS cleans it up:
    shm_unlink(posixShm.c_str());

    return 0;
}

#endif