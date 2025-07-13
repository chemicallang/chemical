// Copyright (c) Chemical Language Foundation 2025.

#include "MultiFunctionNode.h"
#include "ast/structures/FunctionDeclaration.h"

FunctionDeclaration* MultiFunctionNode::func_for_call(ASTAllocator& allocator, std::vector<Value*>& args) {
    for(auto func : functions) {
        if(func->expectedArgsSize() == args.size() && func->satisfy_args(allocator, args)) {
            return func;
        }
    }
    return nullptr;
}

OverridableFuncHandlingResult handle_name_overload_function(
        ASTAllocator& astAllocator,
        ASTNode* previous_node,
        FunctionDeclaration* declaration
) {
    OverridableFuncHandlingResult result;
    auto previous = previous_node->as_function();
    auto multi = previous_node->as_multi_func_node();
    if(multi) {
        const auto prev_spec = multi->specifier();
        if(prev_spec.has_value() && prev_spec.value() != declaration->specifier()) {
            result.specifier_mismatch = true;
            return result;
        }
        bool failed = false;
        for(auto func : multi->functions) {
            if(func->do_param_types_match(declaration->params)) {
                result.duplicates.emplace_back(func);
                failed = true;
            }
        }
        if(failed) return result;
        declaration->set_multi_func_index(multi->functions.size());
        multi->functions.emplace_back(declaration);
    } else if(previous) {
        if(previous->parent() != declaration->parent()) {
            return result;
        }
        if(!previous->do_param_types_match(declaration->params)) {
            multi = new (astAllocator.allocate<MultiFunctionNode>()) MultiFunctionNode(declaration->name_view(), declaration->ASTNode::parent(), declaration->ASTNode::encoded_location());
            multi->functions.emplace_back(previous);
            multi->functions.emplace_back(declaration);
            declaration->set_multi_func_index(1);
            result.new_multi_func_node = multi;
        } else {
            result.duplicates.emplace_back(previous_node);
        }
    } else {
        result.duplicates.emplace_back(previous_node);
    }
    return result;
}
