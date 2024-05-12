// Copyright (c) Qinetik 2024.

#include "ASTProcessor.h"
#include "SelfInvocation.h"
#include <iostream>
#include "ast/base/ASTNode.h"

ASTProcessor::ASTProcessor(std::string curr_exe_path, const std::string& path) : curr_exe_path(std::move(curr_exe_path)), current_path(path), path(path) {

}


std::string ASTProcessor::headers_dir(const std::string& header) {

    if(system_headers_paths.empty()) {
        system_headers_paths = std::move(::system_headers_path(curr_exe_path));
    }

    return ::headers_dir(system_headers_paths, header);

}

void ASTProcessor::info(const std::string &err, ASTNode *node) {
    std::string errStr = "[Codegen] Info\n";
    errStr += "---- message : " + err + "\n";
    errStr += "---- file path : " + current_path;
#ifdef DEBUG
    std::cout << errStr;
    if(node) {
        std::cout << "\n" << "---- node representation : " + node->representation();
    }
    std::cout << std::endl;
#endif
}

void ASTProcessor::error(const std::string &err, ASTNode *node) {
    std::string errStr = "[Codegen] ERROR\n";
    errStr += "---- message : " + err + "\n";
#ifdef DEBUG
    std::cerr << errStr;
    if(node) {
        std::cerr << "\n" << "---- node representation : " + node->representation();
    }
    std::cerr << std::endl;
#endif
    errors.push_back(errStr);
}

void ASTProcessor::print_errors() {
    for (const auto &err: errors) {
        std::cerr << err << std::endl;
    }
}