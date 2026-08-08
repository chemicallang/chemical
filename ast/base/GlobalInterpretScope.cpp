// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 06/03/2024.
//

#include "GlobalInterpretScope.h"

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
    // Global scope should not destruct values - it's reused and outlives individual interpretations
    should_destruct_values = false;
}

GlobalInterpretScope::~GlobalInterpretScope() = default;