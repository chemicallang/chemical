// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 06/03/2024.
//

#include "GlobalInterpretScope.h"

// The one definition of the per-thread call stack (declared in the header).
// Keeping it out of the header means only this object emits the thread_local
// init/destructor helper, so the MinGW links no longer see duplicate weak
// symbols for it.
thread_local std::vector<FunctionCall*> GlobalInterpretScope::call_stack;

GlobalInterpretScope::GlobalInterpretScope(
    OutputMode mode,
    TargetData& target_data,
    BackendContext* context,
    LabBuildCompiler* buildCompiler,
    ASTAllocator& allocator,
    TypeBuilder& typeBuilder,
    LocationManager& loc_man,
    bool interpretation_mode
) : ASTDiagnoser(loc_man), InterpretScope(nullptr, allocator, this), mode(mode), target_data(target_data),
    backend_context(context), build_compiler(buildCompiler), allocator(allocator), typeBuilder(typeBuilder),
    interpretation_mode(interpretation_mode) {
}

GlobalInterpretScope::~GlobalInterpretScope() = default;