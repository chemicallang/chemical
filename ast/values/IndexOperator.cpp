// Copyright (c) Qinetik 2024.

#include "IndexOperator.h"
#include "ast/base/ASTNode.h"
#include "ast/structures/StructDefinition.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "ast/types/ArrayType.h"

llvm::Value *IndexOperator::elem_pointer(Codegen &gen, ASTNode *arr) {
    std::vector<llvm::Value *> idxList;
    auto type = arr->llvm_type(gen);
    if (type->isArrayTy()) {
        idxList.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), 0));
    }
    for(auto& value : values) {
        idxList.emplace_back(value->llvm_value(gen));
    }
    idxList.shrink_to_fit();
    return gen.builder->CreateGEP(type, arr->llvm_pointer(gen), idxList, "", gen.inbounds);
}

llvm::Value *IndexOperator::llvm_pointer(Codegen &gen) {
    return elem_pointer(gen, linked);
}

llvm::Value *IndexOperator::llvm_value(Codegen &gen) {
    return gen.builder->CreateLoad(linked->llvm_elem_type(gen), elem_pointer(gen, linked), "arr0");
}

bool IndexOperator::add_member_index(Codegen &gen, ASTNode *parent, std::vector<llvm::Value *> &indexes) {
    if (parent && !identifier->add_member_index(gen, parent, indexes)) {
        gen.error("couldn't add child index in index operator for identifier " + identifier->representation());
        return false;
    }
    if (!linked->add_child_indexes(gen, indexes, values)) {
        gen.error("couldn't add child index in index operator for identifier " + identifier->representation());
        return false;
    }
    return true;
}

llvm::Type *IndexOperator::llvm_type(Codegen &gen) {
    auto value_type = linked->create_value_type();
    if (value_type->kind() == BaseTypeKind::Array) {
        return ((ArrayType *) value_type.get())->elem_type->llvm_type(gen);
    } else {
        gen.error("cannot get element type from a variable that's not an array");
        return nullptr;
    }
}

#endif

std::unique_ptr<BaseType> IndexOperator::create_type() const {
    auto value_type = linked->create_value_type();
    if (value_type->kind() == BaseTypeKind::Array) {
        return std::unique_ptr<BaseType>(((ArrayType *) value_type.get())->elem_type->copy());
    } else {
        // TODO report error here
        return nullptr;
    }
}

void IndexOperator::link(SymbolResolver &linker) {
    identifier->link(linker);
    linked = identifier->linked_node();
    if (linked) {
        for(auto& value : values) {
            value->link(linker);
        }
    } else {
        linker.error("no identifier with name '" + identifier->representation() + "' found to link for index operator");
    }
}

ASTNode *IndexOperator::linked_node() {
    if (!linked) return nullptr;
    auto value = values[0].get();
    if(values.size() > 1) {
        std::cerr << "Index operator only supports a single integer index at the moment" << std::endl;
        return nullptr;
    }
    return linked->child(value->value_type() == ValueType::Int ? value->as_int() : -1);
}

Value *IndexOperator::find_in(InterpretScope &scope, Value *parent) {
    auto value = values[0].get();
    if(values.size() > 1) {
        scope.error("Index operator only supports a single integer index at the moment");
    }
#ifdef DEBUG
    try {
        return parent->index(scope, value->evaluated_value(scope)->as_int());
    } catch (...) {
        std::cerr << "[InterpretError] index operator only support's integer indexes at the moment";
    }
#endif
    parent->index(scope, value->evaluated_value(scope)->as_int());
    return nullptr;
}

ASTNode *IndexOperator::find_link_in_parent(ASTNode *parent) {
    linked = identifier->find_link_in_parent(parent);
    return linked;
}