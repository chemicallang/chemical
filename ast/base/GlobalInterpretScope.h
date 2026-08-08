// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 06/03/2024.
//

#pragma once

#include "InterpretScope.h"
#include "ComptimeReturnHandler.h"
#include "ASTAllocator.h"
#include "compiler/ASTDiagnoser.h"
#include <vector>
#include <memory>
#include <unordered_map>
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

class StructValue;

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
     * whether the current job is an interpretation job
     * stored as a flag to avoid chasing build_compiler->current_job->type
     */
    bool interpretation_mode = false;

    /**
     * set to true when a comptime function evaluation is triggered
     * from codegen (2c or LLVM), as opposed to running in the
     * pure interpretation mode
     */
    bool is_runtime_call = false;

    /**
     * set to true by the backend's return handler while it is translating a
     * returned runtime value (inside handle_return_value).
     *
     * used to intercept nested comptime function calls that are being
     * translated from within the handler's visit (e.g. a comptime call inside
     * a %runtime_block_value's scope), where the nested return value can only
     * be translated while the nested interpret scope is still alive
     */
    bool in_return_handler = false;

    /**
     * when set, a return statement of the top most comptime function that is
     * called from the runtime mode (codegen) will be handled by this handler,
     * implemented by the backend (C translator or LLVM backend).
     *
     * the handler translates the returned runtime value while the interpret
     * scope where the return is being interpreted is still alive, so captured
     * comptime variables can be resolved
     */
    ComptimeReturnHandler* return_handler = nullptr;

    /**
     * the interpret scope where the top most comptime function's return is
     * being interpreted. the backend's return handler sets this right before
     * translating the returned runtime value and restores it afterwards, so
     * the scope (and the parameters/locals stored in it) stays alive while
     * the value is translated, and is only destroyed after translation.
     *
     * identifiers that are linked with a CapturedComptimeVariable (a variable
     * auto captured from the comptime environment, referenced inside a
     * %runtime_value) resolve their value through this scope. this is used by
     * the backend's own identifier translation (C translator / LLVM) and by
     * the interpreter when it evaluates such identifiers, for example a nested
     * comptime intrinsic call like intrinsics::size invoked from within the
     * returned expression
     */
    InterpretScope* current_capture_scope = nullptr;

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
     * Break value for loop value expressions (e.g., var j = loop { break i; }).
     * Stored on the global scope so it survives child scope destruction (if/switch
     * bodies create child scopes that are destroyed before the loop's body scope
     * can read the break value).
     */
    Value* loop_break_value = nullptr;

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
        LocationManager& loc_man,
        bool interpretation_mode = false
    );

    /**
     * returns the ast diagnoser
     */
    inline ASTDiagnoser& getASTDiagnoser() noexcept {
        return *this;
    }

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
     * overrides the destructor of InterpretScope
     * this is done because dereferencing "this" in base class for an object of derived class
     * causes segfaults, which could be because of object slicing
     */
    ~GlobalInterpretScope() final;

};