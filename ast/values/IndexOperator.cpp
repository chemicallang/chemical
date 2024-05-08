// Copyright (c) Qinetik 2024.

#include "IndexOperator.h"
#include "ast/base/ASTNode.h"
#include "ast/structures/StructDefinition.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "ast/types/ArrayType.h"

llvm::Value *IndexOperator::elem_pointer(Codegen &gen, llvm::Type *type, llvm::Value *ptr) {
    std::vector<llvm::Value *> idxList;
    if (type->isArrayTy()) {
        idxList.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), 0));
    }
    for (auto &value: values) {
        idxList.emplace_back(value->llvm_value(gen));
    }
    idxList.shrink_to_fit();
    return gen.builder->CreateGEP(type, ptr, idxList, "", gen.inbounds);
}

llvm::Value *IndexOperator::elem_pointer(Codegen &gen, ASTNode *arr) {
    return elem_pointer(gen, arr->llvm_type(gen), arr->llvm_pointer(gen));
}

llvm::Value *IndexOperator::llvm_pointer(Codegen &gen) {
    return elem_pointer(gen, parent_val->linked_node());
}

llvm::Value *IndexOperator::llvm_value(Codegen &gen) {
    if(parent_val->value_type() == ValueType::String) {
        auto ptr = elem_pointer(gen, gen.builder->getInt8Ty(), parent_val->llvm_value(gen));
        return gen.builder->CreateLoad(gen.builder->getInt8Ty(), ptr);
    } else {
        return gen.builder->CreateLoad(llvm_type(gen), elem_pointer(gen, parent_val->linked_node()),"idx_op");
    }
}

bool IndexOperator::add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) {
    if (parent->value_type() == ValueType::Array) {
        indexes.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), 0));
    }
    for (auto &value: values) {
        indexes.emplace_back(value->llvm_value(gen));
    }
    return true;
}

llvm::Type *IndexOperator::llvm_type(Codegen &gen) {
    return parent_val->create_type()->create_child_type()->llvm_type(gen);
}

llvm::FunctionType *IndexOperator::llvm_func_type(Codegen &gen) {
    return create_type()->llvm_func_type(gen);
}

#endif

bool IndexOperator::is_direct_value_ref() {
    return parent_val->value_type() == ValueType::String;
}

std::unique_ptr<BaseType> IndexOperator::create_type() const {
    return parent_val->create_type()->create_child_type();
}

void IndexOperator::link(SymbolResolver &linker) {
    throw std::runtime_error("Cannot link a index operator that does not have a identifier");
}

ASTNode *IndexOperator::linked_node() {
    auto value_type = parent_val->create_type();
    return ((ArrayType *) value_type.get())->elem_type->linked_node();
}

Value *IndexOperator::find_in(InterpretScope &scope, Value *parent) {
    auto value = values[0].get();
    if (values.size() > 1) {
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

void IndexOperator::find_link_in_parent(Value *parent) {
    parent_val = parent;
}

std::string IndexOperator::representation() const {
    std::string rep("[");
    unsigned i = 0;
    while (i < values.size()) {
        rep.append(values[i]->representation());
        if (i < values.size() - 1) {
            rep.append(1, ',');
        }
        i++;
    }
    rep.append(1, ']');
    return rep;
}