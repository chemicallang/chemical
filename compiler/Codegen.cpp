// Copyright (c) Qinetik 2024.

#include "ast/base/ASTNode.h"
#include "Codegen.h"

Codegen::Codegen(std::vector<std::unique_ptr<ASTNode>> nodes, std::string path): nodes(std::move(nodes)), path(std::move(path)) {
    module_init();
}

void Codegen::compile() {
    for(const auto& node : nodes) {
        node->code_gen(*this);
        position++;
    }
}

void Codegen::error(const std::string& err){
    std::string errStr = "[Codegen] ERROR\n";
    errStr += "---- message : " + err + "\n";
    errStr += "---- node representation : " + nodes[position]->representation() + '\n';
    errStr += "---- node position : " + std::to_string(position);
    errors.push_back(errStr);
}