// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#include "GlobalInterpretScope.h"
#include "ast/base/Value.h"
#include "ast/base/ASTNode.h"
#include "ast/structures/Scope.h"
#include "rang.hpp"
#include <utility>
#include <iostream>

GlobalInterpretScope::GlobalInterpretScope(
    BackendContext* context,
    LabBuildCompiler* buildCompiler,
    ASTAllocator& allocator
) : InterpretScope(nullptr, this), backend_context(context), build_compiler(buildCompiler), allocator(allocator) {

}

void GlobalInterpretScope::interpret_error(const std::string& msg, ASTAny* any) {
#ifdef DEBUG
    std::cerr << rang::fg::red << "[InterpretError] " << msg << rang::fg::reset << std::endl;
#endif
    ASTDiagnoser::error(msg, any);
}

void GlobalInterpretScope::clean() {
    InterpretScope::clean();
    diagnostics.clear();
}

GlobalInterpretScope::~GlobalInterpretScope() = default;