// Copyright (c) Qinetik 2024.

#include "IndexOperator.h"
#include "ast/base/ASTNode.h"
#include "ast/structures/StructDefinition.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

// TODO isInBounds optimization, when we know that index is in bounds
llvm::Value* IndexOperator::elem_pointer(Codegen& gen, ASTNode* arr) {
    std::vector<llvm::Value*> idxList;
    auto type = arr->llvm_type(gen);
    if(type->isArrayTy()) {
        idxList.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), 0));
    }
    idxList.push_back(value->llvm_value(gen));
    idxList.shrink_to_fit();
    return gen.builder->CreateGEP(type, arr->llvm_pointer(gen), idxList);;
}

llvm::Value * IndexOperator::llvm_pointer(Codegen &gen) {
    return elem_pointer(gen, linked);
}

llvm::Value * IndexOperator::llvm_value(Codegen &gen) {
    return gen.builder->CreateLoad(linked->llvm_elem_type(gen), elem_pointer(gen, linked), "arr0");
}

#endif

void IndexOperator::link(ASTLinker &linker) {
    auto found = linker.current.find(identifier);
    if (found != linker.current.end()) {
        linked = found->second;
        value->link(linker);
    } else {
        linker.error("no identifier with name '" + identifier + "' found to link for index operator");
    }
}

ASTNode *IndexOperator::linked_node(ASTLinker &linker) {
    if (!linked) link(linker);
    if (!linked) return nullptr;
    return linked->child(value->value_type() == ValueType::Int ? value->as_int() : -1);
}

ASTNode *IndexOperator::find_link_in_parent(ASTNode *parent)  {
    auto found = parent->child(identifier);
    if(found) {
        linked = found;
    }
    return found;
}