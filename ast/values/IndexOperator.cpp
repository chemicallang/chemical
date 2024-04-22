// Copyright (c) Qinetik 2024.

#include "IndexOperator.h"
#include "ast/base/ASTNode.h"
#include "ast/structures/StructDefinition.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "ast/types/ArrayType.h"

// TODO isInBounds optimization, when we know that index is in bounds
llvm::Value *IndexOperator::elem_pointer(Codegen &gen, ASTNode *arr) {
    std::vector<llvm::Value *> idxList;
    auto type = arr->llvm_type(gen);
    if (type->isArrayTy()) {
        idxList.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), 0));
    }
    idxList.push_back(value->llvm_value(gen));
    idxList.shrink_to_fit();
    return gen.builder->CreateGEP(type, arr->llvm_pointer(gen), idxList);
}

llvm::Value *IndexOperator::llvm_pointer(Codegen &gen) {
    return elem_pointer(gen, linked);
}

llvm::Value *IndexOperator::llvm_value(Codegen &gen) {
    return gen.builder->CreateLoad(linked->llvm_elem_type(gen), elem_pointer(gen, linked), "arr0");
}

bool IndexOperator::add_member_index(Codegen &gen, ASTNode *parent, std::vector<llvm::Value *> &indexes) {
    if (parent && !parent->add_child_index(gen, indexes, identifier)) {
        gen.error("couldn't add child index in index operator for identifier " + identifier);
        return false;
    }
    if (value->value_type() != ValueType::Int) {
        gen.error("cannot add index for a non int value type in index operator on identifier " + identifier);
        return false;
    }
    if (!linked->add_child_index(gen, indexes, value->as_int())) {
        gen.error("couldn't add child index in index operator for identifier " + identifier);
        return false;
    }
    return true;
}

llvm::Type *IndexOperator::llvm_type(Codegen &gen) {
    auto value_type = linked->create_value_type();
    if(value_type->kind() == BaseTypeKind::Array) {
        return ((ArrayType*) value_type.get())->elem_type->llvm_type(gen);
    } else {
        gen.error("cannot get element type from a variable that's not an array");
    }
}

#endif

std::unique_ptr<BaseType> IndexOperator::create_type() const {
    auto value_type = linked->create_value_type();
    if(value_type->kind() == BaseTypeKind::Array) {
        return std::unique_ptr<BaseType>(((ArrayType*) value_type.get())->elem_type->copy());
    } else {
        // TODO report error here
        return nullptr;
    }
}

void IndexOperator::link(SymbolResolver &linker) {
    auto found = linker.current.find(identifier);
    if (found != linker.current.end()) {
        linked = found->second;
        value->link(linker);
    } else {
        linker.error("no identifier with name '" + identifier + "' found to link for index operator");
    }
}

ASTNode *IndexOperator::linked_node() {
    if (!linked) return nullptr;
    return linked->child(value->value_type() == ValueType::Int ? value->as_int() : -1);
}

ASTNode *IndexOperator::find_link_in_parent(ASTNode *parent) {
    auto found = parent->child(identifier);
    if (found) {
        linked = found;
    }
    return found;
}