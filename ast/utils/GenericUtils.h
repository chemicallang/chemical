// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include <memory>

class BaseType;

/**
 * get iteration for given generic args, if it exists, otherwise returns -1
 * non generic functions return 0
 */
int16_t get_iteration_for(std::vector<std::unique_ptr<GenericTypeParameter>>& generic_params, std::vector<std::unique_ptr<BaseType>>& generic_list);

/**
 * how many actual functions are generated from this generic function
 * non-generic functions return 1
 */
int16_t total_generic_iterations(std::vector<std::unique_ptr<GenericTypeParameter>>& generic_params);

/**
 * this means it doesn't check for existing usages of the generic, and forces to register
 * an iteration for the given generic args, this should be called carefully only after
 * checking that get_iteration_for(args) == -1 is true (meaning it doesn't exist)
 */
int16_t register_generic_usage_no_check(std::vector<std::unique_ptr<GenericTypeParameter>>& generic_params, std::vector<std::unique_ptr<BaseType>>& generic_list);

/**
 * a call notifies a function, during symbol resolution that it exists
 * when this happens, generics are checked, proper types are registered in generic
 * @return iteration that corresponds to this call
 */
int16_t register_generic_usage(std::vector<std::unique_ptr<GenericTypeParameter>>& generic_params, std::vector<std::unique_ptr<BaseType>>& generic_list);