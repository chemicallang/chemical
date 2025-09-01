// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 06/03/2024.
//

#pragma once

#include "InterpretScope.h"
#include "ASTAllocator.h"
#include "compiler/ASTDiagnoser.h"
#include <vector>
#include <memory>
#include "compiler/lab/TargetData.h"
#include "TypeLoc.h"
#include "compiler/OutputMode.h"

class BackendContext;

class LabBuildCompiler;

class Namespace;

class SymbolResolver;

struct GlobalContainer;

class TypeBuilder;

struct IffyBase;

std::optional<bool> is_condition_enabled(GlobalContainer* container, const chem::string_view& name);

std::optional<bool> is_condition_enabled(GlobalContainer* container, IffyBase* base);

class GlobalInterpretScope final : public InterpretScope, public ASTDiagnoser {
public:

    /**
     * the output mode
     */
    OutputMode mode;

    /**
     * the target data for which we're generating code
     */
    TargetData target_data;

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
     * the current function body being interpreted, this would be
     * nullptr if no function is being interpreted
     */
    FunctionTypeBody* current_func_type = nullptr;

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
     * the type cache is used to get types
     */
    TypeBuilder& typeBuilder;

    /**
     * The constructor
     */
    explicit GlobalInterpretScope(
        OutputMode mode,
        TargetData& target_data,
        BackendContext* backendContext,
        LabBuildCompiler* buildCompiler,
        ASTAllocator& allocator,
        TypeBuilder& typeBuilder,
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
    GlobalContainer* create_container(SymbolResolver& resolver, const TargetData& data);

    /**
     * this global container will be binded to this symbol resolver
     */
    void rebind_container(SymbolResolver& resolver, GlobalContainer* container, const TargetData& data);

    /**
     * the given containe will be disposed
     */
    static void dispose_container(GlobalContainer* container);

    /**
     * a target data, that user allocates can be used to get information about the target triple
     */
    void prepare_target_data(TargetData& data, const std::string& target_triple);

    /**
     * overrides the destructor of InterpretScope
     * this is done because dereferencing "this" in base class for an object of derived class
     * causes segfaults, which could be because of object slicing
     */
    ~GlobalInterpretScope() final;

    /**
     * apart from adding a diagnostic, this notifies in debug mode
     */
    void interpret_error(std::string& error, SourceLocation loc);

    /**
     * apart from adding a diagnostic, this notifies in debug mode
     */
    void interpret_error(std::string_view& error, SourceLocation loc);

    /**
     * apart from adding a diagnostic, this notifies in debug mode
     */
    void interpret_error(std::string& error, ASTNode* any);

    /**
     * apart from adding a diagnostic, this notifies in debug mode
     */
    void interpret_error(std::string_view& error, ASTNode* any);

    /**
     * apart from adding a diagnostic, this notifies in debug mode
     */
    void interpret_error(std::string& error, Value* any);

    /**
     * apart from adding a diagnostic, this notifies in debug mode
     */
    void interpret_error(std::string_view& error, Value* any);

    /**
     * apart from adding a diagnostic, this notifies in debug mode
     */
    void interpret_error(std::string& error, const TypeLoc& any);

    /**
     * apart from adding a diagnostic, this notifies in debug mode
     */
    void interpret_error(std::string_view& error, const TypeLoc& any);

};