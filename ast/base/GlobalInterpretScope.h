// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#pragma once

#include "InterpretScope.h"
#include "utils/fwd/functional.h"
#include "ASTAllocator.h"
#include "compiler/ASTDiagnoser.h"
#include <vector>
#include <memory>
#include "compiler/lab/TargetData.h"

class BackendContext;

class LabBuildCompiler;

class Namespace;

class SymbolResolver;

struct GlobalContainer;

class GlobalInterpretScope : public InterpretScope, public ASTDiagnoser {
public:

    /**
     * the target triple given by the user
     * this is what we're generating code for
     */
    const std::string target_triple;

    /**
     * the current function call is the last one
     */
    std::vector<FunctionCall*> call_stack;

    /**
     * a pointer to build compiler is stored, so compile time
     * function calls can talk to the compiler (get definitions)
     */
    LabBuildCompiler* build_compiler;

    /**
     * a pointer to backend context is stored, so compile time
     * function calls can generate code based on the backend
     */
    BackendContext* backend_context;

    /**
     * Currently InterpretScope
     * 1 - allocates everything on heap directly (no batch allocations) (bad thing)
     * 2 - free's everything when it dies (good thing)
     *
     * If interpret scope must use this allocator, it must allocate memory with it
     * but free it as soon as it is done, however we don't have such implementation
     *
     * DO NOT USE this allocator, as this allocator is just to share memory with interpret
     * scope, interpret scope's allocate must always be called
     */
    ASTAllocator& allocator;

    /**
     * The constructor
     */
    explicit GlobalInterpretScope(
        std::string target_triple,
        BackendContext* backendContext,
        LabBuildCompiler* buildCompiler,
        ASTAllocator& allocator,
        LocationManager& loc_man
    );

    /**
     * deleted copy constructor
     * @param copy
     */
    GlobalInterpretScope(const GlobalInterpretScope &copy) = delete;

    /**
     * use default move constructor
     */
    GlobalInterpretScope(GlobalInterpretScope&& global) = default;

    /**
     * a container is created, which will be disposed, user is responsible for it's
     * ownership
     */
    GlobalContainer* create_container(SymbolResolver& resolver);

    /**
     * this global container will be binded to this symbol resolver
     */
    void rebind_container(SymbolResolver& resolver, GlobalContainer* container);

    /**
     * the given containe will be disposed
     */
    static void dispose_container(GlobalContainer* container);

    /**
     * a target data, that user allocates can be used to get information about the target triple
     */
    void prepare_target_data(TargetData& data);

    /**
     * overrides the destructor of InterpretScope
     * this is done because dereferencing "this" in base class for an object of derived class
     * causes segfaults, which could be because of object slicing
     */
    ~GlobalInterpretScope() final;

    /**
     * apart from adding a diagnostic, this notifies in debug mode
     */
    void interpret_error(std::string& error, ASTAny* any);

    /**
     * apart from adding a diagnostic, this notifies in debug mode
     */
    void interpret_error(std::string_view& error, ASTAny* any);

};