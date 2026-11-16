// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include <memory>
#include <span>
#include "compiler/generics/InstantiationsContainer.h"

class BaseType;

/**
 * get iteration for given generic args, if it exists, otherwise returns -1
 * non generic functions return 0
 */
int16_t get_iteration_for(
        const std::vector<std::span<BaseType*>>& instantiations,
        std::vector<TypeLoc>& generic_list
);

/**
 * get iteration for given generic args, if it exists, otherwise returns -1
 * non generic functions return 0
 */
int16_t get_iteration_for(
    std::vector<GenericTypeParameter*>& generic_params,
    std::vector<TypeLoc>& generic_list
);

/**
 * a call notifies a function, during symbol resolution that it exists
 * when this happens, generics are checked, proper types are registered in generic
 * @return iteration that corresponds to this call and whether it was registered a new
 *
 * this function will put root node of given node on the symbol resolver's map if generic args
 * couldn't find iteration and had to create one
 */
std::pair<int16_t, bool> register_generic_usage(
        ASTAllocator& astAllocator,
        void* key,
        InstantiationsContainer& container,
        std::vector<TypeLoc>& generic_list,
        std::vector<void*>& instVec
);

/**
 * infers type of generic parameter type, by given argument type
 * for example for a function func <T> sum(a : T, b : int) : int
 * user could do sum(10, 20) <-- in this call, we don't need to explicitly
 * give int because 10 argument get's arg_type and sends it to this argument
 * with also the param type which is T (referenced generic type parameter)
 * this function then registers int with T usage
 * any errors are reported in given ASTDiagnoser, debug_value is used for
 * better errors
 */
void infer_types_by_args(
        ASTDiagnoser& diagnoser,
        ASTNode* params_node,
        unsigned int generic_list_size,
        BaseType* param_type,
        TypeLoc arg_type,
        std::vector<TypeLoc>& inferred,
        Value* debug_value
);

/**
 * assumes that out_generic_args has size equals to generic_params.size()
 * this just fixes the out_generic_args to have default types from generic type parameters
 */
bool initialize_generic_args(
        ASTDiagnoser& diagnoser,
        std::vector<TypeLoc>& out_generic_args,
        std::vector<GenericTypeParameter*>& generic_params,
        std::vector<TypeLoc>& user_generic_list
);

/**
 * checks the generic args, if generic arg is nullptr, the default generic
 * arg is checked and used if found
 */
bool check_inferred_generic_args(
        ASTDiagnoser& diagnoser,
        std::vector<TypeLoc>& generic_args,
        std::vector<GenericTypeParameter*>& generic_params,
        SourceLocation location
);

/**
 * True when `type` (or a type nested in it) still refers to a generic type
 * parameter. An argument like that makes a generic type *partially applied*
 * (`Box<T>` written inside the body of a generic that declares `T`); such a
 * type must stay deferred to the generic instantiation pass. A fully concrete
 * instantiation (`Box<int>`) contains no generic parameter and can be
 * materialized immediately.
 */
bool type_mentions_generic_param(const BaseType* type);

/**
 * The runtime name of an instantiation is derived from its *content* — the
 * generic declaration's module-qualified name plus its concrete type arguments —
 * instead of from the position of the instantiation in the declaring module's
 * `instantiations` vector.
 *
 * The position is only unique *within* a module, and every module instantiates
 * its own copies, so two different types could print one name (the C compiler
 * then keeps whichever body it saw first and the program is wrong, and a
 * whole-program C build is nondeterministic because the numbering depends on how
 * the parallel symbol-resolution pass interleaves files). A content-derived name
 * is the same in every module that reaches the instantiation — which is what
 * makes a cross-module call to it resolve — and identical between two builds,
 * which is what makes the per-module translation cache safe.
 *
 * `owner` is the generic declaration's implementation (the master
 * `StructDefinition`/`FunctionDeclaration`/...); `args` are the concrete type
 * arguments. Both are hashed with FNV-1a 64; callers store the result on the
 * instantiation (`generic_mangle_hash`) and `NameMangler` prints it.
 *
 * Determinism requirements, which the walk in GenericUtils.cpp implements:
 *  - every named type is spelled by its module-qualified name, never by a
 *    pointer or a source location;
 *  - a nested instantiation contributes its own content hash, so `Box<Option<int>>`
 *    and `Box<Option<string>>` differ (the nested hash is set before the outer
 *    instantiation's signature is finalized, see `register_generic_args`).
 */
uint64_t instantiation_mangle_hash(ASTNode* owner, const std::vector<TypeLoc>& args);