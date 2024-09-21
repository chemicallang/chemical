// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#include "GlobalInterpretScope.h"
#include "ast/base/Value.h"
#include "ast/base/ASTNode.h"
#include "ast/structures/Scope.h"

#include <utility>
#include <iostream>

#define ANSI_COLOR_RED     "\x1b[91m"
#define ANSI_COLOR_RESET   "\x1b[0m"

GlobalInterpretScope::GlobalInterpretScope(
    BackendContext* context,
    LabBuildCompiler* buildCompiler,
    ASTAllocator& allocator
) : InterpretScope(nullptr, this), backend_context(context), build_compiler(buildCompiler), allocator(allocator) {

}

void GlobalInterpretScope::add_error(const std::string &err) {
#ifdef DEBUG
    std::cerr << ANSI_COLOR_RED << "[InterpretError] " << err << ANSI_COLOR_RESET << std::endl;
#endif
    errors.emplace_back(err);
}

void GlobalInterpretScope::clean() {
    InterpretScope::clean();
    errors.clear();
}

GlobalInterpretScope::~GlobalInterpretScope() = default;