// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include <memory>
#include <span>

class BaseType;

bool are_all_specialized(const std::span<BaseType*>& types);

/**
 * get iteration for given generic args, if it exists, otherwise returns -1
 * non generic functions return 0
 */
int16_t get_iteration_for(
    std::vector<GenericTypeParameter*>& generic_params,
    std::vector<BaseType*>& generic_list
);

/**
 * how many actual functions are generated from this generic function
 * non-generic functions return 1
 */
int16_t total_generic_iterations(std::vector<GenericTypeParameter*>& generic_params);

/**
 * this means it doesn't check for existing usages of the generic, and forces to register
 * an iteration for the given generic args, this should be called carefully only after
 * checking that get_iteration_for(args) == -1 is true (meaning it doesn't exist)
 */
int16_t register_generic_usage_no_check(
    ASTAllocator& allocator,
    std::vector<GenericTypeParameter*>& generic_params,
    std::vector<BaseType*>& generic_list
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
    std::vector<GenericTypeParameter*>& generic_params,
    std::vector<BaseType*>& generic_list
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
        BaseType* arg_type,
        std::vector<BaseType*>& inferred,
        Value* debug_value
);