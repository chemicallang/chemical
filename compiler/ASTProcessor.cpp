// Copyright (c) Qinetik 2024.

#include "ASTProcessor.h"
#include "SelfInvocation.h"
#include <iostream>
#include "ast/base/ASTNode.h"

#define ANSI_COLOR_RESET   "\x1b[0m"

ASTProcessor::ASTProcessor(std::string curr_exe_path, const std::string& path) : curr_exe_path(std::move(curr_exe_path)), current_path(path), path(path) {

}


std::string ASTProcessor::headers_dir(const std::string& header) {

    if(system_headers_paths.empty()) {
        system_headers_paths = std::move(::system_headers_path(curr_exe_path));
    }

    return ::headers_dir(system_headers_paths, header);

}

void ASTProcessor::info(const std::string &err, ASTNode *node) {
    std::string errStr = "[" + TAG() + "]\n";
    errStr += "---- message : " + err + "\n";
    errStr += "---- file path : " + current_path;
    if(node) {
        errStr += "---- node representation : " + node->representation();
    }
#ifdef DEBUG
//    std::cerr << errStr << std::endl;
#endif
    errors.emplace_back(errStr, DiagSeverity::Information);
}

void ASTProcessor::error(const std::string &err, ASTNode *node) {
    has_errors = true;
    std::string errStr = "[" + TAG() + "]\n";
    errStr += "---- message : " + err + "\n";
    errStr += "---- file path : " + current_path;
    if(node) {
        errStr += "---- node representation : " + node->representation();
    }
#ifdef DEBUG
//    std::cerr << errStr << std::endl;
#endif
    errors.emplace_back(errStr, DiagSeverity::Error);
}

void ASTProcessor::print_errors() {
    std::cout << "[" << TAG() << "] " << std::to_string(errors.size()) << " diagnostics gathered" << std::endl;
    for (const auto &err: errors) {
        std::cout << color(err.severity) << err.message << ANSI_COLOR_RESET << std::endl;
    }
}