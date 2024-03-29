// Copyright (c) Qinetik 2024.

#include "AccessChain.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

void AccessChain::code_gen(Codegen &gen) {
    for(const auto& value : values) {
        value->code_gen(gen);
    }
}

llvm::Value * AccessChain::llvm_value(Codegen &gen) {
    return values[values.size() - 1]->llvm_value(gen);
//        gen.error("Unimplemented accessing complete access chain as llvm value");
//        return nullptr;
}

llvm::Value * AccessChain::llvm_pointer(Codegen &gen) {
    return values[values.size() - 1]->llvm_pointer(gen);
//        gen.error("Unimplemented accessing complete access chain as llvm pointer");
//        return nullptr;
}

#endif

void AccessChain::declare_and_link(ASTLinker &linker) {
    link(linker);
}

void AccessChain::link(ASTLinker &linker) {
    if(values.size() == 1) {
        values[0]->link(linker);
    } else {
        auto parent = values[0]->linked_node(linker);
        if(!parent) {
            linker.error("couldn't find fragment '" + values[0]->representation() + "' in access chain '" + representation() +"'");
            return;
        }
        unsigned i = 1;
        while(i < values.size()) {
            parent = values[i]->find_link_in_parent(parent);
            if(!parent) {
                linker.error("couldn't find fragment '" + values[i]->representation() + "' in access chain '" + representation() +"'");
                break;
            }
            i++;
        }
    }
}