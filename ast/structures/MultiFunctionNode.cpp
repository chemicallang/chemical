// Copyright (c) Qinetik 2024.

#include "MultiFunctionNode.h"
#include "compiler/SymbolResolver.h"
#include "ast/structures/FunctionDeclaration.h"

MultiFunctionNode::MultiFunctionNode(std::string name) : name(std::move(name)) {

}

SourceLocation MultiFunctionNode::encoded_location() {
    return functions[0]->encoded_location();
}

FunctionDeclaration* MultiFunctionNode::func_for_call(ASTAllocator& allocator, std::vector<Value*>& args) {
    for(auto func : functions) {
        if(func->satisfy_args(allocator, args)) {
            return func;
        }
    }
    return nullptr;
}

void MultiFunctionNode::declare_and_link(SymbolResolver &linker) {

    // link all the functions
    for(auto func : functions) {
        func->declare_and_link(linker);
    }

}

OverridableFuncHandlingResult handle_name_overload_function(
        const chem::string_view& name,
        ASTNode* previous_node,
        FunctionDeclaration* declaration
) {
    OverridableFuncHandlingResult result { {}, nullptr };
    auto previous = previous_node->as_function();
    auto multi = previous_node->as_multi_func_node();
    if(multi) {
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
        if(previous->parent_node != declaration->parent_node) {
            return result;
        }
        if(!previous->do_param_types_match(declaration->params)) {
            // TODO VERY IMPORTANT Multi function node allocated without allocator
            multi = new MultiFunctionNode(name.str());
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
