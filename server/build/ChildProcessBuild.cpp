// Copyright (c) Chemical Language Foundation 2025.

#include "ChildProcessBuild.h"
#include "compiler/lab/LabBuildContext.h"
#include "server/build/ipc_process.h"
#include "server/build/ContextSerialization.h"
#include <iostream>
#include <vector>

#ifdef _WIN32

#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <errno.h>
#include <cstring>
#include <ctime>
#include <pthread.h>
#endif

int report_context_to_parent(BasicBuildContext& context, const std::string& shmName, const std::string& evtChild, const std::string& evtParent) {
    auto jsonStr = labBuildContext_toJsonStr(context);
    return child_create_and_write_shm(shmName, evtChild, evtParent, jsonStr);
}

#ifdef _WIN32

int launch_child_build(BasicBuildContext& context, const std::string_view& lspPath, const std::string_view& buildFilePath) {

    // 1) Generate unique names:
    std::string shmName, evtChildDone, evtParentAck;
    make_unique_names(shmName, evtChildDone, evtParentAck);

    // 2) Create the “parent‐ack” event BEFORE launching child:
    std::wstring wAck = utf8_to_wide(evtParentAck);
    HANDLE hAckEvt = CreateEventW(
            nullptr,      // no security
            FALSE,        // auto-reset
            FALSE,        // initially nonsignaled
            wAck.c_str()  // name
    );
    if (!hAckEvt) {
        DWORD e = GetLastError();
        std::cerr << "[Parent] CreateEventW(parentAck) failed (GLE=" << e << ")\n";
        return 1;
    }

    std::vector<std::string> argv;
    argv.emplace_back(lspPath);
    argv.emplace_back("--build-lab");
    argv.emplace_back(buildFilePath);
    argv.emplace_back("--shmName");
    argv.emplace_back(shmName);
    argv.emplace_back("--evtChildDone");
    argv.emplace_back(evtChildDone);
    argv.emplace_back("--evtParentAck");
    argv.emplace_back(evtParentAck);

    HANDLE hChildProc = nullptr;
    if (!launch_child_process(argv, hChildProc)) {
        CloseHandle(hAckEvt);
        return 1;
    }

    // 4) Parent now creates “childDone” event handle and waits up to timeout:
    std::wstring wChildDone = utf8_to_wide(evtChildDone);
    HANDLE hChildEvt = CreateEventW(
            nullptr,
            FALSE,   // auto-reset
            FALSE,   // initially nonsignaled
            wChildDone.c_str()
    );
    if (!hChildEvt) {
        DWORD e = GetLastError();
        std::cerr << "[Parent] CreateEventW(childDone) failed (GLE=" << e << ")\n";
        TerminateProcess(hChildProc, 1);
        CloseHandle(hChildProc);
        CloseHandle(hAckEvt);
        return 1;
    }

    const DWORD timeoutMs = 10'000;
    DWORD wait = WaitForSingleObject(hChildEvt, timeoutMs);
    ChildResult result;
    if (wait == WAIT_TIMEOUT) {
        std::cerr << "[Parent] Timeout waiting for child to signal.\n";
        // Kill child:
        TerminateProcess(hChildProc, 1);
        CloseHandle(hChildProc);
        result.reason = ChildReason::TIMEOUT;
        result.exitCode = -1;
        // Attempt to read whatever‐if‐any mapping exists:
        if (parent_read_and_cleanup_windows(shmName, evtParentAck, result)) {
            // payload (maybe partial or empty) is in result.payload
        }
    }
    else if (wait == WAIT_OBJECT_0) {
        // Child signaled “done”:
        // 1) Fetch its exit code:
        DWORD code = 0;
        GetExitCodeProcess(hChildProc, &code);
        CloseHandle(hChildProc);
        if (code == 0) {
            result.reason = ChildReason::SUCCESS;
            result.exitCode = 0;
        } else {
            result.reason = ChildReason::CRASH;
            result.exitCode = static_cast<int>(code);
        }
        // 2) Read the mapping and signal “parentAck”:
        if (!parent_read_and_cleanup_windows(shmName, evtParentAck, result)) {
            // Reading failed:
            // But we still have result.reason / exitCode set
        }
    }
    else {
        DWORD e = GetLastError();
        std::cerr << "[Parent] WaitForSingleObject(childDone) failed (GLE=" << e << ")\n";
        TerminateProcess(hChildProc, 1);
        CloseHandle(hChildProc);
        CloseHandle(hChildEvt);
        CloseHandle(hAckEvt);
        return 1;
    }

    // 5) Clean up event handles:
    CloseHandle(hChildEvt);
    CloseHandle(hAckEvt);

    // 6) Report results:
    switch (result.reason) {
        case ChildReason::SUCCESS:
            std::cout << "[Parent] Child succeeded (exit=0)\n";
            break;
        case ChildReason::CRASH:
            std::cout << "[Parent] Child crashed (exit=" << result.exitCode << ")\n";
            break;
        case ChildReason::TIMEOUT:
            std::cout << "[Parent] Child timed out\n";
            break;
    }

    if(result.payload.empty()) {
        std::cerr << "[lsp] received empty string from child process" << std::endl;
        return 1;
    }

    const auto ok = labBuildContext_fromJson(context, result.payload);

    // 6) Print the string:
    std::cout << "[lsp] Shared‐memory contents:\n---\n"
              << result.payload
              <<  std::endl;

    return ok ? 0 : 1;

}

#else

int launch_child_build(BasicBuildContext& context, const std::string_view& lspPath, const std::string_view& buildFilePath) {

    // 1) Generate unique names:
    std::string shmName, evtChildDone, evtParentAck;
    make_unique_names(shmName, evtChildDone, evtParentAck);

    // 2) Parent creates “parent_ack” semaphore (initially 0):
    //    POSIX semaphores require a leading '/'. We’ll prepend if needed.
    std::string semAckName = evtParentAck;
    if (semAckName.empty() || semAckName[0] != '/')
        semAckName = "/" + semAckName;

    // Unlink in case it already existed from a prior run:
    sem_unlink(semAckName.c_str());
    sem_t *semParentAck = sem_open(
            semAckName.c_str(),
            O_CREAT | O_EXCL,
            0600,
            0   // initial value = 0
    );
    if (semParentAck == SEM_FAILED) {
        std::cerr << "[Parent] sem_open(parentAck) failed: " << strerror(errno) << "\n";
        return 1;
    }

    std::vector<std::string> argv;
    argv.emplace_back(lspPath);
    argv.emplace_back("--build-lab");
    argv.emplace_back(buildFilePath);
    argv.emplace_back("--shmName");
    argv.emplace_back(shmName);
    argv.emplace_back("--evtChildDone");
    argv.emplace_back(evtChildDone);
    argv.emplace_back("--evtParentAck");
    argv.emplace_back(evtParentAck);

    pid_t childPid;
    if (!launch_child_process(argv, childPid)) {
        sem_close(semParentAck);
        sem_unlink(semAckName.c_str());
        return 1;
    }

    // 4) Parent opens “child_done” semaphore and waits up to timeout:
    std::string semChildName = evtChildDone;
    if (semChildName.empty() || semChildName[0] != '/')
        semChildName = "/" + semChildName;
    // Unlink in case it existed:
    sem_unlink(semChildName.c_str());
    sem_t *semChildDone = sem_open(
            semChildName.c_str(),
            O_CREAT | O_EXCL,
            0600,
            0  // initial = 0
    );
    if (semChildDone == SEM_FAILED) {
        std::cerr << "[Parent] sem_open(childDone) failed: " << strerror(errno) << "\n";
        kill(childPid, SIGKILL);
        waitpid(childPid, nullptr, 0);
        sem_close(semParentAck);
        sem_unlink(semAckName.c_str());
        return 1;
    }

    // Wait for the child to post (sem_post) on semChildDone, with timeout:
    const int timeoutMs = 10'000;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeoutMs / 1000;
    ts.tv_nsec += (timeoutMs % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000;
    }

    int semRes = sem_timedwait(semChildDone, &ts);
    ChildResult result;
    if (semRes == -1 && errno == ETIMEDOUT) {
        std::cerr << "[Parent] Timeout waiting for child to post.\n";
        // Kill child:
        kill(childPid, SIGKILL);
        waitpid(childPid, nullptr, 0);
        result.reason = ChildReason::TIMEOUT;
        result.exitCode = -1;
        // Attempt to read shared memory anyway:
        parent_read_shared_memory(shmName, result);
    }
    else if (semRes == 0) {
        // Child signaled “done”:
        int status = 0;
        waitpid(childPid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            result.reason = ChildReason::SUCCESS;
            result.exitCode = 0;
        } else if (WIFEXITED(status)) {
            result.reason = ChildReason::CRASH;
            result.exitCode = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            result.reason = ChildReason::CRASH;
            result.exitCode = 128 + WTERMSIG(status);
        } else {
            result.reason = ChildReason::CRASH;
            result.exitCode = -1;
        }
        // Read shared memory now:
        parent_read_shared_memory(shmName, result);

        // Signal parentAck so child can finish cleaning up:
        sem_post(semParentAck);
    }
    else {
        std::cerr << "[Parent] sem_timedwait(childDone) failed: " << strerror(errno) << "\n";
        kill(childPid, SIGKILL);
        waitpid(childPid, nullptr, 0);
        sem_close(semChildDone);
        sem_close(semParentAck);
        sem_unlink(semChildName.c_str());
        sem_unlink(semAckName.c_str());
        return 1;
    }

    // 5) Clean up semaphores and shm:
    sem_close(semChildDone);
    sem_unlink(semChildName.c_str());
    sem_close(semParentAck);
    sem_unlink(semAckName.c_str());

    std::string shmPath = shmName;
    if (shmPath.empty() || shmPath[0] != '/') shmPath = "/" + shmPath;
    shm_unlink(shmPath.c_str());

    // 6) Report:
    switch (result.reason) {
        case ChildReason::SUCCESS:
            std::cout << "[Parent] Child succeeded (exit=0)\n";
            break;
        case ChildReason::CRASH:
            std::cout << "[Parent] Child crashed (exit=" << result.exitCode << ")\n";
            break;
        case ChildReason::TIMEOUT:
            std::cout << "[Parent] Child timed out\n";
            break;
    }

    if(result.payload.empty()) {
        std::cerr << "[lsp] received empty string from child process" << std::endl;
        return 1;
    }

    const auto ok = labBuildContext_fromJson(context, result.payload);

    // 6) Print the string:
    std::cout << "[lsp] Shared‐memory contents:\n---\n"
              << result.payload
              <<  std::endl;

    return ok ? 0 : 1;

}

#endif