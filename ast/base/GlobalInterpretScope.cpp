// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#include "GlobalInterpretScope.h"
#include "ast/base/Value.h"
#include "ast/base/ASTNode.h"
#include "ast/base/BaseType.h"
#include "ast/structures/Scope.h"
#include "rang.hpp"
#include <utility>
#include <iostream>

GlobalInterpretScope::GlobalInterpretScope(
    std::string target_triple,
    BackendContext* context,
    LabBuildCompiler* buildCompiler,
    ASTAllocator& allocator,
    LocationManager& loc_man
) : ASTDiagnoser(loc_man), InterpretScope(nullptr, allocator, this), target_triple(std::move(target_triple)),
    backend_context(context), build_compiler(buildCompiler), allocator(allocator) {

}

void GlobalInterpretScope::interpret_error(std::string& msg, SourceLocation loc) {
#ifdef DEBUG
    std::cerr << rang::fg::red << "[InterpretError] " << msg << rang::fg::reset << std::endl;
#endif
    ASTDiagnoser::diagnostic(msg, loc, DiagSeverity::Error);
}

void GlobalInterpretScope::interpret_error(std::string_view& msg, SourceLocation loc) {
#ifdef DEBUG
    std::cerr << rang::fg::red << "[InterpretError] " << msg << rang::fg::reset << std::endl;
#endif
    ASTDiagnoser::diagnostic(msg, loc, DiagSeverity::Error);
}

void GlobalInterpretScope::interpret_error(std::string& error, ASTNode* any) {
    interpret_error(error, any->encoded_location());
}

void GlobalInterpretScope::interpret_error(std::string_view& error, ASTNode* any) {
    interpret_error(error, any->encoded_location());
}

void GlobalInterpretScope::interpret_error(std::string& error, Value* any) {
    interpret_error(error, any->encoded_location());
}

void GlobalInterpretScope::interpret_error(std::string_view& error, Value* any) {
    interpret_error(error, any->encoded_location());
}

void GlobalInterpretScope::interpret_error(std::string& error, BaseType* any) {
    interpret_error(error, any->encoded_location());
}

void GlobalInterpretScope::interpret_error(std::string_view& error, BaseType* any) {
    interpret_error(error, any->encoded_location());
}

GlobalInterpretScope::~GlobalInterpretScope() = default;