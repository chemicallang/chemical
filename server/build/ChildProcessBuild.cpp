// Copyright (c) Chemical Language Foundation 2025.

#include "ChildProcessBuild.h"
#include "compiler/lab/LabBuildContext.h"
#include "server/build/ipc_process.h"
#include "server/build/ContextSerialization.h"
#include <iostream>
#include <vector>

int report_context_to_parent(BasicBuildContext& context, const std::string& shmName) {
    auto jsonStr = labBuildContext_toJsonStr(context);
    const auto ok = child_create_and_write_shm(shmName, jsonStr);
    return ok ? 0 : 1;
}

int launch_child_build(BasicBuildContext& context, const std::string_view& lspPath, const std::string_view& buildFilePath) {

    // 1) Generate a unique shared‐memory name:
    std::string shmName = generate_shm_name();

    std::vector<std::string> argv;
    argv.emplace_back(lspPath);
    argv.emplace_back("--build-lab");
    argv.emplace_back(buildFilePath);   // replace with your real build file path
    argv.emplace_back("--shmName");
    argv.emplace_back(shmName);

    // 3) Launch:
    PROCESS_HANDLE childHandle;
    if (!launch_child_process(argv, childHandle)) {
        std::cerr << "[lsp] Failed to launch child process for compiling '" << buildFilePath << '\'' << std::endl;
        return 1;
    }

    // 4) Wait up to 10 000 ms:
    ChildResult result;
    if (!wait_for_child_and_read(childHandle, shmName, 10'000, result)) {
        std::cerr << "[lsp] Error while waiting for child or reading shared memory" << std::endl;
        return 1;
    }

    // 5) Check how it ended:
    switch (result.reason) {
        case ChildReason::SUCCESS:
            std::cout << "[lsp] Child exited successfully." << std::endl;
            break;
        case ChildReason::CRASH:
            std::cout << "[lsp] Child crashed (exit=" << result.exitCode << ")." << std::endl;
            break;
        case ChildReason::TIMEOUT:
            std::cout << "[lsp] Child timed out / was killed." << std::endl;
            break;
    }

    if(result.resultString.empty()) {
        std::cerr << "[lsp] received empty string from child process" << std::endl;
        return 1;
    }

    const auto ok = labBuildContext_fromJson(context, result.resultString);

    // 6) Print the string:
    std::cout << "[lsp] Shared‐memory contents:\n---\n"
              << result.resultString
              <<  std::endl;

#ifndef _WIN32
    // Clean up POSIX shm:
std::string posixName = shmName;
if (posixName[0] != '/') posixName = "/" + posixName;
shm_unlink(posixName.c_str());
#endif

    return ok ? 0 : 1;

}