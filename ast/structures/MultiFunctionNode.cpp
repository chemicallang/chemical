// Copyright (c) Qinetik 2024.

#include "MultiFunctionNode.h"
#include "compiler/SymbolResolver.h"
#include "ast/structures/FunctionDeclaration.h"

MultiFunctionNode::MultiFunctionNode(std::string name) : name(std::move(name)) {

}

CSTToken *MultiFunctionNode::cst_token() {
    return functions[0]->cst_token();
}

FunctionDeclaration* MultiFunctionNode::func_for_call(std::vector<std::unique_ptr<Value>>& args) {
    for(auto func : functions) {
        if(func->satisfy_args(args)) {
            return func;
        }
    }
    return nullptr;
}

void MultiFunctionNode::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {

    // TODO Multi Function Node doesn't allow replacing functions when linking
    std::unique_ptr<ASTNode> dummy;

    // link all the functions
    for(auto func : functions) {
        func->declare_and_link(linker, dummy);
    }

}

OverridableFuncHandlingResult handle_name_overload_function(
        const std::string& name,
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
        declaration->multi_func_index = multi->functions.size();
        multi->functions.emplace_back(declaration);
    } else if(previous) {
        if(previous->parent_node != declaration->parent_node) {
            return result;
        }
        if(!previous->do_param_types_match(declaration->params)) {
            multi = new MultiFunctionNode(name);
            multi->functions.emplace_back(previous);
            multi->functions.emplace_back(declaration);
            declaration->multi_func_index = 1;
            result.new_multi_func_node = multi;
        } else {
            result.duplicates.emplace_back(previous_node);
        }
    } else {
        result.duplicates.emplace_back(previous_node);
    }
    return result;
}
