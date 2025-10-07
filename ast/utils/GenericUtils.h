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
void default_generic_args(
        std::vector<TypeLoc>& out_generic_args,
        std::vector<GenericTypeParameter*>& generic_params,
        std::vector<TypeLoc>& user_generic_list
);