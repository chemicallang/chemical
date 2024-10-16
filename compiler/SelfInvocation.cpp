// Copyright (c) Qinetik 2024.

#include "SelfInvocation.h"
#include "utils/PathUtils.h"
#include <iostream>
#include <sstream>
#include <filesystem>

int invoke_capturing_out(const std::vector<std::string> &command_args, std::string &output) {
    std::stringstream buffer;
    FILE *pipe = nullptr;

    std::string command;
    for (const auto &arg : command_args) {
        command += arg + " ";
    }
    command += " 2>&1"; // Redirect stderr to stdout

#ifdef _WIN32
    if ((pipe = _popen(command.c_str(), "r")) == nullptr) {
        std::cerr << "error _popen failed in clang invocation" << std::endl;
        return -1; // Error handling, return an appropriate error code
    }
#else
    if ((pipe = popen(command.c_str(), "r")) == nullptr) {
        return -1; // Error handling, return an appropriate error code
    }
#endif

    char temp_buffer[500];
    while (fgets(temp_buffer, 500, pipe) != nullptr) {
        buffer << temp_buffer;
    }

#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif

    output = buffer.str();
    return 0;
}

std::vector<std::string> system_headers_path(const std::string &arg0) {

    std::vector<std::string> commands{arg0, "cc", "-v","-c","-xc++"};
#ifdef WIN32
    commands.emplace_back("nul");
#else
    commands.emplace_back("/dev/null");
#endif

    std::string output;
    invoke_capturing_out(commands, output);

#ifndef WIN32
    // WARNING : Clang on linux creates this null.o file
    // even though we gave /dev/null BUT STILL, can you believe it !!!!
    // its probably in the current working dir
    try {
        std::remove("null.o");
    } catch (...) {
        // don't do anything
    }
#endif

//    std::cout << " OUTPUT : " << output << std::endl;

    std::vector<std::string> paths;
    std::string prefix = "#include <...> search starts here:";
    std::string suffix = "End of search list.";

    size_t start = output.find(prefix);
    size_t end = output.find(suffix, start);
    if (start != std::string::npos && end != std::string::npos) {
        start += prefix.length();
        std::string pathList = output.substr(start, end - start);

        size_t pos = 0;
        while ((pos = pathList.find('\n', pos)) != std::string::npos) {
            pos++; // Move past the newline character
            size_t nextPos = pathList.find('\n', pos);
            std::string path = pathList.substr(pos, nextPos - pos);
            // Trim leading and trailing whitespaces
            size_t firstNonSpace = path.find_first_not_of(" \t");
            size_t lastNonSpace = path.find_last_not_of(" \t");
            if (firstNonSpace != std::string::npos && lastNonSpace != std::string::npos) {
                paths.push_back(path.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1));
            }
            pos = nextPos;
        }
    }

//    std::cout << "Paths:";
//    for(const auto& path1 : paths) {
//        std::cout << path1 << std::endl;
//    }

    return paths;
}

std::string header_abs_path(std::vector<std::string>& system_headers, const std::string& header) {
    for(const auto& sys_head : system_headers) {
        std::filesystem::path abs_path = std::filesystem::path(sys_head) / header;
        if (std::filesystem::exists(abs_path)) {
            return abs_path.string();
        }
    }
    return "";
}

std::string headers_dir(std::vector<std::string>& system_headers, const std::string& header) {
    for(const auto& sys_head : system_headers) {
        std::filesystem::path abs_path = std::filesystem::path(sys_head) / header;
        if (std::filesystem::exists(abs_path)) {
            return sys_head;
        }
    }
    return "";
}